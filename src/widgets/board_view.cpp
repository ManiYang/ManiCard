#include <utility>
#include <QDebug>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "app_data.h"
#include "board_view.h"
#include "models/custom_data_query.h"
#include "models/data_view_box_data.h"
#include "models/settings/card_label_color_mapping.h"
#include "persisted_data_access.h"
#include "services.h"
#include "utilities/action_debouncer.h"
#include "utilities/async_routine.h"
#include "utilities/binary_search.h"
#include "utilities/colors_util.h"
#include "utilities/geometry_util.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "utilities/margins_util.h"
#include "utilities/message_box.h"
#include "utilities/numbers_util.h"
#include "utilities/periodic_checker.h"
#include "utilities/strings_util.h"
#include "widgets/board_view_toolbar.h"
#include "widgets/components/data_view_box.h"
#include "widgets/components/edge_arrow.h"
#include "widgets/components/graphics_scene.h"
#include "widgets/components/group_box.h"
#include "widgets/components/node_rect.h"
#include "widgets/components/setting_box.h"
#include "widgets/dialogs/dialog_create_relationship.h"
#include "widgets/dialogs/dialog_set_labels.h"
#include "widgets/widgets_constants.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

BoardView::BoardView(QWidget *parent)
        : QFrame(parent) {
    handleSettingsEditedDebouncer = new ActionDebouncer(
            1000, ActionDebouncer::Option::Delay,
            [this]() { settingBoxesCollection.handleEditedSettings(); }, this);

    //
    setUpWidgets();
    setUpConnections();
    installEventFiltersOnComponents();
}

void BoardView::loadBoard(
        const int boardIdToLoad,
        std::function<void (bool loadOk, bool highlightedCardIdChanged)> callback) {
    if (boardId == boardIdToLoad) {
        callback(true, false);
        return;
    }

    // close all items
    bool highlightedCardIdChanged = false; // to -1

    if (boardId != -1) {
        if (!canClose()) {
            callback(false, false);
            return;
        }

        closeAll(&highlightedCardIdChanged);
        boardId = -1;
    }

    if (boardIdToLoad == -1) {
        callback(true, highlightedCardIdChanged);
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList userLabelsList;
        Board board;
        QHash<int, Card> cardsData;
        QHash<RelationshipId, RelationshipProperties> relationshipsData;
        QHash<int, CustomDataQuery> customDataQueriesData;
    };
    auto *routine = new AsyncRoutineWithVars;
    routine->setName("BoardView::loadBoard");

    boardId = boardIdToLoad; // will be set to -1 (in final step) if failed to load

    routine->addStep([this, routine]() {
        // 0. get the list of user-defined labels
        using StringListPair = std::pair<QStringList, QStringList>;
        Services::instance()->getAppDataReadonly()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userLabelsList = labelsAndRelTypes.first;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, boardIdToLoad]() {
        // 1. get board data
        Services::instance()->getAppDataReadonly()->getBoardData(
                boardIdToLoad,
                // callback
                [routine](bool ok, std::optional<Board> board) {
                    ContinuationContext context(routine);

                    if (!ok || !board.has_value())
                        context.setErrorFlag();
                    else
                        routine->board = board.value();
                },
                this
        );
    }, this);

    routine->addStep([this, routine, boardIdToLoad]() {
        // 2. get cards data
        const QSet<int> cardIds = keySet(routine->board.cardIdToNodeRectData);
        Services::instance()->getAppDataReadonly()->queryCards(
                cardIds,
                // callback
                [routine, cardIds, boardIdToLoad](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }

                    if (keySet(cards) != cardIds) {
                        qWarning().noquote()
                                << QString("not all cards exist for board %1").arg(boardIdToLoad);
                    }
                    routine->cardsData = cards;
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 3. get relationships data
        using RelId = RelationshipId;
        using RelProperties = RelationshipProperties;

        Services::instance()->getAppDataReadonly()->queryRelationshipsFromToCards(
                keySet(routine->cardsData),
                // callback
                [routine](bool ok, const QHash<RelId, RelProperties> &rels) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }

                    for (auto it = rels.constBegin(); it != rels.constEnd(); ++it) {
                        const RelId &relId = it.key();
                        if (!routine->cardsData.contains(relId.startCardId)
                                || !routine->cardsData.contains(relId.endCardId)) {
                            continue;
                        }
                        routine->relationshipsData.insert(relId, it.value());
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 4. get custom-data-queries data
        const QSet<int> customDataQueryIds
                = keySet(routine->board.customDataQueryIdToDataViewBoxData);

        Services::instance()->getAppDataReadonly()->queryCustomDataQueries(
                customDataQueryIds,
                // callback
                [routine](bool ok, const QHash<int, CustomDataQuery> &customDataQueries) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }
                    routine->customDataQueriesData = customDataQueries;
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 5. create NodeRect's, EdgeArrow's, DataViewBox's, GroupBox's, RelationshipsBundle's
        ContinuationContext context(routine);

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

        // NodeRect's
        CardPropertiesToShow effectiveCardPropertiesToShow
                = cardPropertiesToShowSettings.onWorkspace;
        effectiveCardPropertiesToShow.updateWith(routine->board.cardPropertiesToShow);

        for (auto it = routine->cardsData.constBegin();
                it != routine->cardsData.constEnd(); ++it) {
            const int &cardId = it.key();
            const Card &cardData = it.value();

            const NodeRectData nodeRectData = routine->board.cardIdToNodeRectData.value(cardId);

            const QColor displayColor = computeNodeRectDisplayColor(
                    nodeRectData.ownColor, cardData.getLabels(),
                    cardLabelsAndAssociatedColors, defaultNodeRectColor,
                    autoAdjustCardColorsForDarkTheme && isDarkTheme);

            const QString propertiesDisplay = computeCardPropertiesDisplay(
                    effectiveCardPropertiesToShow,
                    cardData.getLabels(), cardData.getCustomProperties());

            NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                    cardId, cardData, nodeRectData.rect,
                    displayColor, nodeRectData.ownColor,
                    routine->userLabelsList, propertiesDisplay);
            nodeRect->setEditable(true);
        }

        // EdgeArrow's
        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = getEdgeArrowLineColor();
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
            edgeArrowData.labelColor = getEdgeArrowLabelColor();

        }

        const auto relIds = keySet(routine->relationshipsData);
        for (const auto &relId: relIds) {
            edgeArrowData.joints = routine->board.relIdToJoints.value(relId);
            relationshipsCollection.createEdgeArrow(relId, edgeArrowData);
        }

        // DataViewBox's
        for (auto it = routine->customDataQueriesData.constBegin();
                it != routine->customDataQueriesData.constEnd(); ++it) {
            const int &customDataQueryId = it.key();
            const CustomDataQuery customDataQuery = it.value();

            const DataViewBoxData dataViewBoxData
                    = routine->board.customDataQueryIdToDataViewBoxData.value(customDataQueryId);

            const QColor displayColor = computeDataViewBoxDisplayColor(
                    dataViewBoxData.ownColor, QColor());

            auto *box = dataViewBoxesCollection.createDataViewBox(
                    customDataQueryId, customDataQuery, dataViewBoxData.rect,
                    displayColor, dataViewBoxData.ownColor);
            box->setEditable(true);
        }

        // GroupBox's
        const QHash<int, GroupBoxData> &groupBoxIdToData = routine->board.groupBoxIdToData;
        for (auto it = groupBoxIdToData.constBegin(); it != groupBoxIdToData.constEnd(); ++it) {
            const int groupBoxId = it.key();
            const GroupBoxData &groupBoxData = it.value();

            Q_ASSERT(groupBoxId != -1);
            GroupBox *groupBox = groupBoxesCollection.createGroupBox(groupBoxId, groupBoxData);

            // -- check that child items' rects are within `groupBox`
            const auto childGroupBoxes = it.value().childGroupBoxes;
            const auto childCards = it.value().childCards;

            //
            for (const int childGroupBoxId: childGroupBoxes) {
                const auto childRect = groupBoxIdToData.value(childGroupBoxId).rect;
                if (childRect.isNull())
                    continue;
                if (!groupBox->getContentsRect().contains(childRect)) {
                    qWarning().noquote()
                            << QString("group-box %1 does not enclose its child group-box %2")
                               .arg(groupBoxId).arg(childGroupBoxId);
                }
            }
            for (const int childCardId: childCards) {
                const auto childRect = routine->board.cardIdToNodeRectData.value(childCardId).rect;
                if (childRect.isNull())
                    continue;
                if (!groupBox->getContentsRect().contains(childRect)) {
                    qWarning().noquote()
                            << QString("group-box %1 does not enclose its child card %2")
                               .arg(groupBoxId).arg(childCardId);
                }
            }
        }

        // GroupBoxTree
        using ChildGroupBoxesAndCards = std::pair<QSet<int>, QSet<int>>;
        QHash<int, ChildGroupBoxesAndCards> treeNodeToChildItems;

        for (auto it = groupBoxIdToData.constBegin(); it != groupBoxIdToData.constEnd(); ++it) {
            treeNodeToChildItems.insert(
                    it.key(), {it.value().childGroupBoxes, it.value().childCards});
        }

        QString errorMsg;
        const bool ok = groupBoxTree.set(treeNodeToChildItems, &errorMsg);
        if (!ok) {
            qWarning().noquote() << "Could not create group-boxes tree:" << errorMsg;
            context.setErrorFlag();
        }

        // RelationshipsBundle's
        updateRelationshipBundles();
    }, this);

    routine->addStep([this, routine, callback]() {
        // 6. create SettingBox's
        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();
        const QColor settingBoxDisplayColor = computeSettingBoxDisplayColor(
                defaultNewSettingBoxColor, autoAdjustCardColorsForDarkTheme && isDarkTheme);

        //
        auto *innerRoutine = new AsyncRoutine;

        const auto settingBoxesData = routine->board.settingBoxesData;
        for (const SettingBoxData &settingBoxData: settingBoxesData) {
            innerRoutine->addStep([this, innerRoutine, settingBoxData, settingBoxDisplayColor]() {
                settingBoxesCollection.createSettingBox(
                        {settingBoxData.targetType, settingBoxData.category},
                        settingBoxData.rect, settingBoxDisplayColor,
                        // callback
                        [innerRoutine](SettingBox */*settingBox*/) {
                            innerRoutine->nextStep();
                        }
                );
            }, this);
        }

        innerRoutine->addStep([innerRoutine, routine]() {
            // final step of `innerRoutine`
            innerRoutine->nextStep();
            routine->nextStep();
        }, this);

        innerRoutine->start();
    }, this);

    routine->addStep([this, routine, callback]() {
        ContinuationContext context(routine);

        cardPropertiesToShowSettings.onBoard = routine->board.cardPropertiesToShow;

        zoomScale = routine->board.zoomRatio;
        canvas->setScale(zoomScale * graphicsGeometryScaleFactor); // (1)
        adjustSceneRect(); // (2)
        setViewTopLeftPos(routine->board.topLeftPos); // (3)
    }, this);

    routine->addStep([this, routine, callback, highlightedCardIdChanged]() {
        // final step
        ContinuationContext context(routine);

        bool highlightedCardIdChanged1 = highlightedCardIdChanged;
        if (routine->errorFlag) {
            boardId = -1;

            bool highlightedCardIdChanged2;
            closeAll(&highlightedCardIdChanged2);

            highlightedCardIdChanged1 |= highlightedCardIdChanged2;
        }

        callback(!routine->errorFlag, highlightedCardIdChanged1);
    }, this);

    routine->start();
}

void BoardView::prepareToClose() {
    handleSettingsEditedDebouncer->actNow();
}

void BoardView::applyZoomAction(const ZoomAction zoomAction) {
    const auto anchorScenePos = getViewCenterInScene();
    doApplyZoomAction(zoomAction, anchorScenePos);
}

void BoardView::toggleCardPreview() {
    const int singleHighlightedCardId // can be -1
            = Services::instance()->getAppDataReadonly()->getSingleHighlightedCardId();
    if (singleHighlightedCardId == -1 || !nodeRectsCollection.contains(singleHighlightedCardId))
        return;
    nodeRectsCollection.get(singleHighlightedCardId)->togglePreview();
}

void BoardView::setColorsAssociatedWithLabels(
        const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &defaultNodeRectColor) {
    this->cardLabelsAndAssociatedColors = cardLabelsAndAssociatedColors;
    this->defaultNodeRectColor = defaultNodeRectColor;

    nodeRectsCollection.updateAllNodeRectColors();
}

void BoardView::cardPropertiesToShowSettingOnWorkspaceUpdated(
        const CardPropertiesToShow &workspaceSettingOfCardPropertiesToShow) {
    cardPropertiesToShowSettings.onWorkspace = workspaceSettingOfCardPropertiesToShow;
    updatePropertiesDisplayOfAllCards();
}

void BoardView::updateSettingBoxOnWorkspaceSetting(
        const int workspaceId, const SettingCategory category) {
    settingBoxesCollection.updateSettingBoxIfExists(
            {SettingTargetType::Workspace, category}, workspaceId);
}

int BoardView::getBoardId() const {
    return boardId;
}

QPointF BoardView::getViewTopLeftPos() const {
    const auto topLeftPosInScene = graphicsView->mapToScene(0, 0);
    return canvas->mapFromScene(topLeftPosInScene);
}

double BoardView::getZoomRatio() const {
    return zoomScale;
}

bool BoardView::canClose() const {
    return true;
}

QImage BoardView::renderAsImage() {
    constexpr double margin = 20;
    const QRectF contentsRectInCanvas
            = getContentsRectInCanvasCoordinates().marginsAdded(uniformMarginsF(margin));

    // set canvas scale to 1.0
    const double originalScale = canvas->scale();
    canvas->setScale(1.0);

    //
    const QPointF topLeftInScene = canvas->mapToScene(contentsRectInCanvas.topLeft());
    const QPointF bottomRightInScene = canvas->mapToScene(contentsRectInCanvas.bottomRight());
            // in scene coordinates
    const QRectF contentsRectInScene(
            QPointF(nearestInteger(topLeftInScene.x()), nearestInteger(topLeftInScene.y())),
            QPointF(nearestInteger(bottomRightInScene.x()), nearestInteger(bottomRightInScene.y()))
    );

    QImage image(
            nearestInteger(contentsRectInScene.width()),
            nearestInteger(contentsRectInScene.height()),
            QImage::Format_ARGB32);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    graphicsScene->render(&painter, QRectF(), contentsRectInScene);

    // resume to original canvas scale
    canvas->setScale(originalScale);

    //
    return image;
}

bool BoardView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == graphicsView) {
        if (event->type() == QEvent::Resize)
            adjustSceneRect();
    }
    return false;
}

void BoardView::setUpWidgets() {
    this->setFrameShape(QFrame::NoFrame);

    // set up layout
    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    this->setLayout(layout);
    {
        graphicsView = new QGraphicsView;
        layout->addWidget(graphicsView);
    }

    // set up `graphicsScene`, & `canvas`
    graphicsScene = new GraphicsScene(this);
    graphicsScene->setItemIndexMethod(QGraphicsScene::NoIndex);

    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    graphicsScene->setBackgroundBrush(getSceneBackgroundColor(isDarkTheme));

    canvas = new QGraphicsRectItem;
    canvas->setFlag(QGraphicsItem::ItemHasNoContents, true);
    graphicsScene->addItem(canvas);

    // set up `graphicsView`
    graphicsView->setScene(graphicsScene);

    graphicsView->setRenderHint(QPainter::Antialiasing, true);
    graphicsView->setRenderHint(QPainter::TextAntialiasing, true);
    graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);

    graphicsView->setFrameShape(QFrame::NoFrame);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void BoardView::setUpConnections() {
    // Connections from newly created NodeRect's are established in
    // NodeRectsCollection::createNodeRect().

    //
    connect(graphicsScene, &GraphicsScene::contextMenuRequestedOnScene,
            this, [this](const QPointF &scenePos) {
        contextMenu.requestScenePos = scenePos;
        contextMenu.setActionIcons();
        contextMenu.menu->popup(getScreenPosFromScenePos(scenePos));
    });

    connect(graphicsScene, &GraphicsScene::clickedOnBackground, this, [this]() {
        onBackgroundClicked();
    });

    connect(graphicsScene, &GraphicsScene::userToZoomInOut,
            this, [this](bool zoomIn, const QPointF &anchorScenePos) {
        doApplyZoomAction(
                zoomIn ? ZoomAction::ZoomIn : ZoomAction::ZoomOut,
                anchorScenePos);
    });

    connect(graphicsScene, &GraphicsScene::viewScrollingStarted, this, [this]() {
        nodeRectsCollection.setAllNodeRectsTextEditorIgnoreWheelEvent(true);
        dataViewBoxesCollection.setAllDataViewBoxesTextEditorIgnoreWheelEvent(true);
        settingBoxesCollection.setAllSettingBoxesTextEditorIgnoreWheelEvent(true);
    });

    connect(graphicsScene, &GraphicsScene::viewScrollingFinished, this, [this]() {
        nodeRectsCollection.setAllNodeRectsTextEditorIgnoreWheelEvent(false);
        dataViewBoxesCollection.setAllDataViewBoxesTextEditorIgnoreWheelEvent(false);
        settingBoxesCollection.setAllSettingBoxesTextEditorIgnoreWheelEvent(false);
    });

    // AppData
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::cardPropertiesUpdated,
            this,
            [this](EventSource eventSrc, const int cardId,
                    const CardPropertiesUpdate &cardPropertiesUpdate) {
        if (eventSrc.sourceWidget == this)
            return;

        //
        if (!nodeRectsCollection.contains(cardId))
            return;

        Services::instance()->getAppDataReadonly()->queryCards(
                {cardId},
                // callback
                [this, cardId, cardPropertiesUpdate](bool ok, const QHash<int, Card> &cards) {
                    if (!ok) {
                        qWarning().noquote() << "could not get card data";
                        return;
                    }
                    if (!cards.contains(cardId))
                        return;

                    //
                    Card cardData = cards.value(cardId);

                    if (!cardPropertiesUpdate.getCustomProperties().isEmpty()) {
                        // card's custom properties updated
                        CardPropertiesToShow effectiveSetting
                                = cardPropertiesToShowSettings.onWorkspace;
                        effectiveSetting.updateWith(cardPropertiesToShowSettings.onBoard);

                        nodeRectsCollection.updateNodeRectPropertiesDisplay(
                                cardId, cardData.getLabels(), cardData.getCustomProperties(),
                                effectiveSetting);
                    }

                    if (cardPropertiesUpdate.title.has_value()
                            || cardPropertiesUpdate.text.has_value()) {
                        Q_ASSERT(false); // card's title and text should be editable only via BoardView
                    }
                },
                this
        );
    });

    connect(Services::instance()->getAppDataReadonly(),
            &AppDataReadonly::fontSizeScaleFactorChanged,
            this, [this](const QWidget *window, const double factor) {
        if (this->window() != window)
            return;
        if (std::fabs(factor - graphicsGeometryScaleFactor) < 1e-3)
            return;

        graphicsGeometryScaleFactor = factor;
        updateCanvasScale(zoomScale * graphicsGeometryScaleFactor, getViewCenterInScene());
    });

    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {

        //
        graphicsScene->setBackgroundBrush(getSceneBackgroundColor(isDarkTheme));

        //
        nodeRectsCollection.updateAllNodeRectColors();

        //
        const QColor edgeArrowLineColor = getEdgeArrowLineColor();
        const QColor edgeArrowLabelColor = getEdgeArrowLabelColor();

        relationshipsCollection.setLineColorAndLabelColorOfAllEdgeArrows(
                edgeArrowLineColor, edgeArrowLabelColor);
        relationshipBundlesCollection.setLineColorAndLabelColorOfAllEdgeArrows(
                edgeArrowLineColor, edgeArrowLabelColor);

        //
        groupBoxesCollection.setColorOfAllGroupBoxes(computeGroupBoxColor(isDarkTheme));

        //
        settingBoxesCollection.updateAllSettingBoxesColors();
    });

    connect(Services::instance()->getAppDataReadonly(),
            &AppDataReadonly::autoAdjustCardColorsForDarkThemeUpdated,
            this, [this](const bool /*autoAdjust*/) {
         nodeRectsCollection.updateAllNodeRectColors();
         settingBoxesCollection.updateAllSettingBoxesColors();
    });
}

void BoardView::installEventFiltersOnComponents() {
    graphicsView->installEventFilter(this);
}

void BoardView::onUserToOpenExistingCard(const QPointF &scenePos) {
    constexpr int minValue = 0;
    constexpr int step = 1;

    bool ok;
    const int cardId = QInputDialog::getInt(
            this, "Open Card", "Card ID to open:", 0, minValue, INT_MAX, step, &ok);
    if (!ok)
        return;

    // check already opened
    if (nodeRectsCollection.contains(cardId)) {
        QMessageBox::information(this, " ", QString("Card %1 already opened.").arg(cardId));
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList userLabelsList;
        Card cardData;
        NodeRectData nodeRectData;
        QHash<RelationshipId, RelationshipProperties> rels;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get the list of user-defined labels
        using StringListPair = std::pair<QStringList, QStringList>;
        Services::instance()->getAppData()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userLabelsList = labelsAndRelTypes.first;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, cardId]() {
        // query card
        Services::instance()->getAppData()->queryCards(
                {cardId},
                // callback
                [=](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not open card. See logs for details.";
                        return;
                    }

                    if (!cards.contains(cardId)) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("Card %1 not found.").arg(cardId);
                        return;
                    }

                    routine->cardData = cards.value(cardId);;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, cardId, scenePos]() {
        // create NodeRect
        ContinuationContext context(routine);

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

        CardPropertiesToShow effectiveCardPropertiesToShow
                = cardPropertiesToShowSettings.onWorkspace;
        effectiveCardPropertiesToShow.updateWith(cardPropertiesToShowSettings.onBoard);

        //
        routine->nodeRectData.rect = QRectF(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                quantize(defaultNewNodeRectSize, boardSnapGridSize)
        );
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, routine->cardData.getLabels(),
                cardLabelsAndAssociatedColors, defaultNodeRectColor,
                autoAdjustCardColorsForDarkTheme && isDarkTheme);

        const QString propertiesDisplay = computeCardPropertiesDisplay(
                effectiveCardPropertiesToShow,
                routine->cardData.getLabels(), routine->cardData.getCustomProperties());

        auto *nodeRect = nodeRectsCollection.createNodeRect(
                cardId, routine->cardData, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList, propertiesDisplay);
        nodeRect->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine, cardId]() {
        // query relationships
        using RelId = RelationshipId;
        using RelProperties = RelationshipProperties;

        Services::instance()->getAppData()->queryRelationshipsFromToCards(
                {cardId},
                // callback
                [routine, cardId](bool ok, const QHash<RelId, RelProperties> &rels) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Could not get relationships connecting card %1. "
                                          "See logs for details.").arg(cardId);
                        return;
                    }

                    routine->rels = rels;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, cardId]() {
        ContinuationContext context(routine);

        // create EdgeArrow's
        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = getEdgeArrowLineColor();
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
            edgeArrowData.labelColor = getEdgeArrowLabelColor();
        }
        for (auto it = routine->rels.constBegin(); it != routine->rels.constEnd(); ++it) {
            const auto &relId = it.key();

            int otherCardId;
            bool b = relId.connectsCard(cardId, &otherCardId);
            Q_ASSERT(b);
            if (!nodeRectsCollection.contains(otherCardId)) // `otherCardId` not opened in this board
                continue;

            relationshipsCollection.createEdgeArrow(relId, edgeArrowData);
        }

        //
        updateRelationshipBundles();
    }, this);

    routine->addStep([this, routine, cardId]() {
        // call AppDate
        ContinuationContext context(routine);
        Services::instance()->getAppData()->createNodeRect(
                EventSource(this), this->boardId, cardId, routine->nodeRectData);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToCreateNewCard(const QPointF &scenePos) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList userLabelsList;
        int newCardId;
        Card card;
        NodeRectData nodeRectData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 0. get the list of user-defined labels
        using StringListPair = std::pair<QStringList, QStringList>;
        Services::instance()->getAppData()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userLabelsList = labelsAndRelTypes.first;
                },
                this
        );
    }, this);

    routine->addStep([routine]() {
        // 1. request new card ID
        Services::instance()->getAppData()->requestNewCardId(
                // callback:
                [routine](std::optional<int> cardId) {
                    ContinuationContext context(routine);
                    if (cardId.has_value()) {
                        routine->newCardId = cardId.value();
                    }
                    else {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not create new card. See logs for details.";
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([this, routine, scenePos]() {
        // 2. create new NodeRect
        ContinuationContext context(routine);

        routine->card.title = "New Card";
        routine->card.text = "";
        const QSet<QString> cardLabels {};

        //
        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

        CardPropertiesToShow effectiveCardPropertiesToShow
                = cardPropertiesToShowSettings.onWorkspace;
        effectiveCardPropertiesToShow.updateWith(cardPropertiesToShowSettings.onBoard);

        //
        routine->nodeRectData.rect = QRectF(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                quantize(defaultNewNodeRectSize, boardSnapGridSize)
        );
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, cardLabels, cardLabelsAndAssociatedColors,
                defaultNodeRectColor, autoAdjustCardColorsForDarkTheme && isDarkTheme);

        const QString propertiesDisplay = computeCardPropertiesDisplay(
                effectiveCardPropertiesToShow,
                routine->card.getLabels(), routine->card.getCustomProperties());

        NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                routine->newCardId, routine->card, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList, propertiesDisplay);
        nodeRect->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine]() {
        // 3. call AppData
        ContinuationContext context(routine);

        Services::instance()->getAppData()->createNewCardWithId(
                EventSource(this), routine->newCardId, routine->card);
        Services::instance()->getAppData()->createNodeRect(
                EventSource(this), this->boardId, routine->newCardId, routine->nodeRectData);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToDuplicateCard(const QPointF &scenePos) {
    // open dialog
    int cardIdToDuplicate = -1;
    {
        constexpr int minValue = 0;
        constexpr int step = 1;

        bool ok;
        cardIdToDuplicate = QInputDialog::getInt(
                this, "Duplicate Card", "ID of card to duplicate:",
                0, minValue, INT_MAX, step, &ok);
        if (!ok)
            return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Card cardData;
        QStringList userLabelsList;
        int newCardId;
        NodeRectData nodeRectData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine, cardIdToDuplicate]() {
        // get card data of `cardIdToDuplicate`
        Services::instance()->getAppData()->queryCards(
                QSet<int> {cardIdToDuplicate},
                // callback
                [routine, cardIdToDuplicate](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);
                    if (ok) {
                        if (cards.contains(cardIdToDuplicate)) {
                            routine->cardData = cards.value(cardIdToDuplicate);
                        }
                        else {
                            context.setErrorFlag();
                            routine->errorMsg
                                    = QString("Card %1 not found.").arg(cardIdToDuplicate);
                        }
                    }
                    else {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Failed to query card data. See logs for details.");
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // get the list of user-defined labels
        using StringListPair = std::pair<QStringList, QStringList>;
        Services::instance()->getAppData()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userLabelsList = labelsAndRelTypes.first;
                },
                this
        );
    }, this);

    routine->addStep([routine]() {
        // request new card ID
        Services::instance()->getAppData()->requestNewCardId(
                // callback:
                [routine](std::optional<int> cardId) {
                    ContinuationContext context(routine);
                    if (cardId.has_value()) {
                        routine->newCardId = cardId.value();
                    }
                    else {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not create new card. See logs for details.";
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([this, routine, cardIdToDuplicate, scenePos]() {
        // create new NodeRect
        ContinuationContext context(routine);

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

        CardPropertiesToShow effectiveCardPropertiesToShow
                = cardPropertiesToShowSettings.onWorkspace;
        effectiveCardPropertiesToShow.updateWith(cardPropertiesToShowSettings.onBoard);

        //
        QSizeF newNodeRectSize = nodeRectsCollection.contains(cardIdToDuplicate)
                ? nodeRectsCollection.getNodeRectRect(cardIdToDuplicate).value().size()
                : quantize(defaultNewNodeRectSize, boardSnapGridSize);
        routine->nodeRectData.rect = QRectF(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                newNodeRectSize
        );
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, routine->cardData.getLabels(),
                cardLabelsAndAssociatedColors, defaultNodeRectColor,
                autoAdjustCardColorsForDarkTheme && isDarkTheme);

        const QString propertiesDisplay = computeCardPropertiesDisplay(
                effectiveCardPropertiesToShow,
                routine->cardData.getLabels(), routine->cardData.getCustomProperties());

        NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                routine->newCardId, routine->cardData, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList, propertiesDisplay);
        nodeRect->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine]() {
        // call AppData
        ContinuationContext context(routine);

        Services::instance()->getAppData()->createNewCardWithId(
                EventSource(this), routine->newCardId, routine->cardData);
        Services::instance()->getAppData()->createNodeRect(
                EventSource(this), this->boardId, routine->newCardId, routine->nodeRectData);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToCreateNewGroup(const QPointF &scenePos) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newGroupBoxId {-1};
        GroupBoxData groupBoxData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([routine]() {
        // request new group-box ID (using requestNewCardId())
        Services::instance()->getAppData()->requestNewCardId(
                // callback:
                [routine](std::optional<int> cardId) {
                    ContinuationContext context(routine);
                    if (cardId.has_value()) {
                        routine->newGroupBoxId = cardId.value();
                    }
                    else {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get new group-box ID. See logs for details.";
                    }
                },
                routine
        );
    }, this);

    routine->addStep([this, routine, scenePos]() {
        // create new GroupBox
        ContinuationContext context(routine);

        routine->groupBoxData.title = "Group";
        routine->groupBoxData.rect = QRectF(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                quantize(defaultNewNodeRectSize, boardSnapGridSize)
        );

        Q_ASSERT(routine->newGroupBoxId != -1);
        groupBoxesCollection.createGroupBox(routine->newGroupBoxId, routine->groupBoxData);

        adjustSceneRect();

        // -- update `groupBoxTree`
        groupBoxTree.containerNode(GroupBoxTree::rootId)
                .addChildGroupBoxes({routine->newGroupBoxId});
    }, this);

    routine->addStep([this, routine]() {
        // call AppData
        ContinuationContext context(routine);

        Services::instance()->getAppData()->createTopLevelGroupBoxWithId(
                EventSource(this), this->boardId, routine->newGroupBoxId, routine->groupBoxData);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToCreateNewCustomDataQuery(const QPointF &scenePos) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newCustomDataQueryId;
        CustomDataQuery customDataQuery;
        DataViewBoxData dataViewBoxData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([routine]() {
        // request new data-query ID (using requestNewCardId())
        Services::instance()->getAppData()->requestNewCardId(
                // callback:
                [routine](std::optional<int> cardId) {
                    ContinuationContext context(routine);
                    if (cardId.has_value()) {
                        routine->newCustomDataQueryId = cardId.value();
                    }
                    else {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not create new data query. See logs for details.";
                    }
                },
                routine
        );
    }, this);

    routine->addStep([this, routine, scenePos]() {
        // create new DataViewBox
        ContinuationContext context(routine);

        routine->customDataQuery.title = "New Data Query";
        routine->customDataQuery.queryCypher
                = "MATCH (c:Card) \n"
                  "WHERE c.id IN $cardIdsOfBoard \n"
                  "RETURN c.id AS id, c.title AS title;";
        routine->customDataQuery.queryParameters = QJsonObject();

        routine->dataViewBoxData.rect = QRectF(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                quantize(defaultNewDataViewBoxSize, boardSnapGridSize)
        );
        routine->dataViewBoxData.ownColor = QColor();

        const QColor displayColor = routine->dataViewBoxData.ownColor.isValid()
                ? routine->dataViewBoxData.ownColor : defaultNewDataViewBoxColor;

        auto *box = dataViewBoxesCollection.createDataViewBox(
                routine->newCustomDataQueryId, routine->customDataQuery,
                routine->dataViewBoxData.rect,
                displayColor, routine->dataViewBoxData.ownColor);
        box->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine]() {
        // call AppData
        ContinuationContext context(routine);

        Services::instance()->getAppData()->createNewCustomDataQueryWithId(
                EventSource(this), routine->newCustomDataQueryId, routine->customDataQuery);

        Services::instance()->getAppData()->createDataViewBox(
                EventSource(this), this->boardId, routine->newCustomDataQueryId,
                routine->dataViewBoxData);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToCreateSettingBox(const QPointF &scenePos) {
    // let user select settings target and category
    SettingTargetTypeAndCategory selectedTargetTypeAndCategory;
    {
        const auto [targetTypeAndCategoryPairs, optionNames]
                = getTargetTypeAndCategoryDisplayNames();
        if (targetTypeAndCategoryPairs.isEmpty())
            return;

        bool ok;
        const QString selectedOptionName = QInputDialog::getItem(
                this, "Open Settings", "Select the settings to open:",
                optionNames,
                0, // initial index
                false, // editable
                &ok);
        if (!ok)
            return;

        const int index = optionNames.indexOf(selectedOptionName);
        Q_ASSERT(index != -1);
        selectedTargetTypeAndCategory = targetTypeAndCategoryPairs.at(index);
    }

    // check whether the selected one is already opened
    if (settingBoxesCollection.containsSetting(selectedTargetTypeAndCategory)) {
        QMessageBox::information(this, "Info", "The selected settings category is already opened.");
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        bool settingBoxCreated {false};
        QRectF rect;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, scenePos, selectedTargetTypeAndCategory]() {
        // create SettingsBox
        QRectF rect(
                quantize(canvas->mapFromScene(scenePos), boardSnapGridSize),
                quantize(defaultNewSettingBoxSize, boardSnapGridSize)
        );
        routine->rect = rect;

        QColor displayColor;
        {
            const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
            const bool autoAdjustCardColorsForDarkTheme
                    = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();
            displayColor = computeSettingBoxDisplayColor(
                    defaultNewSettingBoxColor, autoAdjustCardColorsForDarkTheme && isDarkTheme);
        }

        settingBoxesCollection.createSettingBox(
                selectedTargetTypeAndCategory, rect, displayColor,
                // callback
                [routine](SettingBox *settingBox) {
                    if (settingBox != nullptr)
                        routine->settingBoxCreated = true;
                    routine->nextStep();
                }
        );
    }, this);

    routine->addStep([this, routine, selectedTargetTypeAndCategory]() {
        // call AppData
        if (routine->settingBoxCreated) {
            SettingBoxData settingBoxData;
            {
                settingBoxData.targetType = selectedTargetTypeAndCategory.first;
                settingBoxData.category = selectedTargetTypeAndCategory.second;
                settingBoxData.rect = routine->rect;
            }
            Services::instance()->getAppData()->createSettingBox(
                    EventSource(this), boardId, settingBoxData);
        }
        routine->nextStep();
    }, this);

    routine->start();
}

void BoardView::onUserToCloseSettingBox(const SettingTargetTypeAndCategory &targetTypeAndCategory) {
    settingBoxesCollection.closeSettingBox(targetTypeAndCategory);

    //
    Services::instance()->getAppData()->removeSettingBox(
            EventSource(this), boardId, targetTypeAndCategory.first, targetTypeAndCategory.second);
}

void BoardView::onUserToSetLabels(const int cardId) {
    Q_ASSERT(nodeRectsCollection.contains(cardId));
    using StringListPair = std::pair<QStringList, QStringList>;

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList userCardLabelsList;
        std::optional<QStringList> updatedLabels;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get user-defined labels list
        Services::instance()->getAppData()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userCardLabelsList = labelsAndRelTypes.first;
                },
                this
        );
    }, this);

    //
    routine->addStep([this, routine, cardId]() {
        // show dialog, get updated labels
        auto *dialog = new DialogSetLabels(
                nodeRectsCollection.get(cardId)->getNodeLabels(),
                routine->userCardLabelsList, this);

        connect(dialog, &QDialog::finished, this, [dialog, routine](int result) {
            if (result == QDialog::Accepted) {
                routine->updatedLabels = dialog->getLabels();
                routine->nextStep();
            }
            else {
                routine->skipToFinalStep();
            }
            dialog->deleteLater();
        });

        dialog->open();
    }, this);

    routine->addStep([this, routine, cardId]() {
        // update labels and color of node rect
        ContinuationContext context(routine);

        const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
        const bool autoAdjustCardColorsForDarkTheme
                = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

        //
        Q_ASSERT(routine->updatedLabels.has_value());
        const QStringList updatedLabels = routine->updatedLabels.value();

        auto *nodeRect = nodeRectsCollection.get(cardId);

        const QColor nodeRectColor = computeNodeRectDisplayColor(
                nodeRectsCollection.getNodeRectOwnColor(cardId),
                QSet<QString>(updatedLabels.cbegin(), updatedLabels.cend()),
                cardLabelsAndAssociatedColors, defaultNodeRectColor,
                autoAdjustCardColorsForDarkTheme && isDarkTheme);

        nodeRect->setNodeLabels(updatedLabels);
        nodeRect->setColor(nodeRectColor);
    }, this);

    routine->addStep([this, routine, cardId]() {
        // update properties display
        Services::instance()->getAppDataReadonly()->queryCards(
                {cardId},
                // callback
                [this, routine, cardId](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        qWarning().noquote() << "could not get card data";
                        return;
                    }
                    if (!cards.contains(cardId)) {
                        return;
                    }

                    //
                    const QSet<QString> updatedLabels (
                            routine->updatedLabels.value().constBegin(),
                            routine->updatedLabels.value().constEnd());

                    const auto customProperties = cards.value(cardId).getCustomProperties();

                    CardPropertiesToShow effectiveSetting
                            = cardPropertiesToShowSettings.onWorkspace;
                    effectiveSetting.updateWith(cardPropertiesToShowSettings.onBoard);

                    nodeRectsCollection.updateNodeRectPropertiesDisplay(
                            cardId, updatedLabels, customProperties, effectiveSetting);
                },
                this
        );
    }, this);

    routine->addStep([this, routine, cardId]() {
        // call AppData
        ContinuationContext context(routine);

        Q_ASSERT(routine->updatedLabels.has_value());
        const auto updatedLabels = routine->updatedLabels.value_or(QStringList {});
        Services::instance()->getAppData()->updateCardLabels(
                EventSource(this),
                cardId,
                QSet<QString>(updatedLabels.constBegin(), updatedLabels.constEnd())
        );
    }, this);

    routine->addStep([routine]() {
        // final step
        ContinuationContext context(routine);
    }, this);

    //
    routine->start();
}

void BoardView::onUserToCreateRelationship(const int cardId) {
    Q_ASSERT(nodeRectsCollection.contains(cardId));
    using StringListPair = std::pair<QStringList, QStringList>;

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList userRelTypesList;
        RelationshipId relIdToCreate {-1, -1, ""};
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get user-defined rel. types list
        Services::instance()->getAppData()->getUserLabelsAndRelationshipTypes(
                // callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->userRelTypesList = labelsAndRelTypes.second;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, cardId]() {
        // show dialog, get relationship to create
        auto *dialog = new DialogCreateRelationship(
                cardId, nodeRectsCollection.get(cardId)->getTitle(),
                routine->userRelTypesList, this);

        connect(dialog, &QDialog::finished, this, [dialog, routine](int result) {
            ContinuationContext context(routine);
            dialog->deleteLater();

            if (result != QDialog::Accepted) {
                context.setErrorFlag();
                return;
            }

            const auto idOpt = dialog->getRelationshipId();
            if (!idOpt.has_value()) {
                context.setErrorFlag();
                return;
            }

            routine->relIdToCreate = idOpt.value();
        });

        dialog->open();
    }, this);

    routine->addStep([this, routine]() {
        // check start/end card exist
        const QSet<int> startEndCards {
            routine->relIdToCreate.startCardId,
            routine->relIdToCreate.endCardId
        };
        Services::instance()->getAppData()->queryCards(
                startEndCards,
                // callback
                [routine, startEndCards](bool ok, QHash<int, Card> cards) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        routine->errorMsg = "Could not query start/end cards. See logs for detail";
                        context.setErrorFlag();
                        return;
                    }

                    if (const auto diff = startEndCards - keySet(cards); !diff.isEmpty()) {
                        QString cardIdsStr;
                        {
                            auto it = diff.constBegin();
                            cardIdsStr = QString::number(*it);
                            if (++it != diff.constEnd())
                                cardIdsStr += QString(" & %1").arg(*it);
                        }
                        routine->errorMsg = QString("Card %1 not found.").arg(cardIdsStr);
                        context.setErrorFlag();
                        return;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // check relationship not already exists
        Services::instance()->getAppData()->queryRelationship(
                routine->relIdToCreate,
                // callback
                [routine](bool ok, const std::optional<RelationshipProperties> &propertiesOpt) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not query relationship. See logs for detail.";
                        return;
                    }

                    if (propertiesOpt.has_value()) {
                        context.setErrorFlag();
                        routine->errorMsg = "Relationship already exists.";
                        return;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        ContinuationContext context(routine);

        if (!nodeRectsCollection.contains(routine->relIdToCreate.startCardId)
                || !nodeRectsCollection.contains(routine->relIdToCreate.endCardId)) {
            // (one of the start/end cards is not opened in this board)
            QMessageBox::information(
                    this, " ",
                    QString("Relationship %1 created.").arg(routine->relIdToCreate.toStringRepr()));
            return;
        }

        // create EdgeArrow
        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = getEdgeArrowLineColor();
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
            edgeArrowData.labelColor = getEdgeArrowLabelColor();
        }
        relationshipsCollection.createEdgeArrow(routine->relIdToCreate, edgeArrowData);

        //
        updateRelationshipBundles();
    }, this);

    routine->addStep([this, routine]() {
        // call AppData
        ContinuationContext context(routine);
        Services::instance()->getAppData()
                ->createRelationship(EventSource(this), routine->relIdToCreate);
    }, this);

    routine->addStep([routine, this]() {
        // final step
        routine->nextStep();

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::onUserToCloseNodeRect(const int cardId) {
    Q_ASSERT(nodeRectsCollection.contains(cardId));

    std::optional<int> updatedHighlightedCardId;
    {
        bool highlightedCardIdChanged;
        constexpr bool removeConnectedEdgeArrows = true;
        nodeRectsCollection.closeNodeRect(
                cardId, removeConnectedEdgeArrows, &highlightedCardIdChanged);

        if (highlightedCardIdChanged)
            updatedHighlightedCardId = -1;

        adjustSceneRect();

        //
        groupBoxTree.removeCardIfExists(cardId);

        //
        updateRelationshipBundles();
    }

    // call AppData
    // -- highlighted card
    if (updatedHighlightedCardId.has_value()) {
        Services::instance()->getAppData()->setSingleHighlightedCardId(
                EventSource(this), updatedHighlightedCardId.value());
    }

    // -- remove NodeRect
    Services::instance()->getAppData()->removeNodeRect(EventSource(this), boardId, cardId);

    // -- joints of EdgeArrow's
    BoardNodePropertiesUpdate update;
    update.relIdToJoints = relationshipsCollection.getRelIdToJoints();

    Services::instance()->getAppData()->updateBoardNodeProperties(
            EventSource(this), boardId, update);
}

void BoardView::onUserToCloseDataViewBox(const int customDataQueryId) {
    Q_ASSERT(dataViewBoxesCollection.contains(customDataQueryId));

    //
    dataViewBoxesCollection.closeDataViewBox(customDataQueryId);
    adjustSceneRect();

    // call AppData
    Services::instance()->getAppData()
            ->removeDataViewBox(EventSource(this), boardId, customDataQueryId);
}

void BoardView::onUserToSetGroupBoxTitle(const int groupBoxId, const QString &newTitle) {
    groupBoxesCollection.get(groupBoxId)->setTitle(newTitle);

    //
    GroupBoxNodePropertiesUpdate update;
    update.title = newTitle;

    Services::instance()->getAppData()
            ->updateGroupBoxProperties(EventSource(this), groupBoxId, update);
}

void BoardView::onUserToRemoveGroupBox(const int groupBoxId) {
    groupBoxesCollection.removeGroupBox(groupBoxId);
    adjustSceneRect();

    //
    groupBoxTree.removeGroupBox(groupBoxId, GroupBoxTree::RemoveOption::ReparentChildren);

    //
    updateRelationshipBundles();

    // call AppData
    Services::instance()->getAppData()
            ->removeGroupBoxAndReparentChildItems(EventSource(this), groupBoxId);
}

void BoardView::onBackgroundClicked() {
    nodeRectsCollection.setHighlightedCardIds({});
    groupBoxesCollection.setHighlightedGroupBoxes({});

    // call AppData
    constexpr int highlightedCardId = -1;
    Services::instance()->getAppData()
            ->setSingleHighlightedCardId(EventSource(this), highlightedCardId);
}

void BoardView::reparentNodeRectInGroupBoxTree(const int cardId, const int newParentGroupBox) {
    const int originalParentGroupBox = groupBoxTree.getParentGroupBoxOfCard(cardId); // can be -1
    if (originalParentGroupBox != newParentGroupBox) {
        if (newParentGroupBox == -1) {
            groupBoxTree.removeCard(cardId);

            Services::instance()->getAppData()->removeNodeRectFromGroupBox(
                    EventSource(this), cardId, originalParentGroupBox);
        }
        else {
            groupBoxTree.addOrReparentCard(cardId, newParentGroupBox);

            Services::instance()->getAppData()->addOrReparentNodeRectToGroupBox(
                    EventSource(this), cardId, newParentGroupBox);
        }
    }

    //
    updateRelationshipBundles();
}

void BoardView::reparentGroupBoxInGroupBoxTree(const int groupBoxId, const int newParentGroupBox) {
    int originalParentGroupBox;
    {
        int originalParent = groupBoxTree.getParentOfGroupBox(groupBoxId);
        originalParentGroupBox
                = (originalParent == GroupBoxTree::rootId) ? -1 : originalParent;
    }
    if (originalParentGroupBox != newParentGroupBox) {
        const int newParentId
                = (newParentGroupBox != -1) ? newParentGroupBox : GroupBoxTree::rootId;
        groupBoxTree.reparentExistingGroupBox(groupBoxId, newParentId);

        Services::instance()->getAppData()->reparentGroupBox(
                EventSource(this), groupBoxId, newParentGroupBox);
    }

    //
    updateRelationshipBundles();
}

void BoardView::closeAll(bool *highlightedCardIdChanged_) {
    *highlightedCardIdChanged_ = false;

    const QSet<int> cardIds = nodeRectsCollection.getAllCardIds();
    for (const int &cardId: cardIds) {
        bool highlightedCardIdChanged;
        constexpr bool removeConnectedEdgeArrows = false;
        nodeRectsCollection.closeNodeRect(
                cardId, removeConnectedEdgeArrows, &highlightedCardIdChanged);

        if (highlightedCardIdChanged)
            *highlightedCardIdChanged_ = true;
    }

    //
    const auto relIds = relationshipsCollection.getAllRelationshipIds();
    relationshipsCollection.removeEdgeArrows(relIds);

    //
    const QSet<int> customDataQueryIds = dataViewBoxesCollection.getAllCustomDataQueryIds();
    for (const int &id: customDataQueryIds)
        dataViewBoxesCollection.closeDataViewBox(id);

    //
    const QSet<int> groupBoxIds = groupBoxesCollection.getAllGroupBoxIds();
    for (const int id: groupBoxIds)
        groupBoxesCollection.removeGroupBox(id);

    //
    const auto settings = settingBoxesCollection.getAllSettingBoxes();
    for (const auto &setting: settings)
        settingBoxesCollection.closeSettingBox(setting);

    //
    groupBoxTree.clear();
    updateRelationshipBundles();
}

void BoardView::adjustSceneRect() {
    QGraphicsScene *scene = graphicsView->scene();
    if (scene == nullptr)
        return;

    const QRectF contentsRectInCanvas
            = getContentsRectInCanvasCoordinates(); // in canvas coordinates
    const QRectF contentsRectInScene = contentsRectInCanvas.isNull()
            ? QRectF(0, 0, 10, 10)
            : QRectF(
                  canvas->mapToScene(contentsRectInCanvas.topLeft()),
                  canvas->mapToScene(contentsRectInCanvas.bottomRight()));

    // finite margins to prevent user from drag-scrolling too far away from contents
    constexpr double fraction = 0.8;
    const double marginX = graphicsView->width() * fraction; // (pixel)
    const double marginY = graphicsView->height() * fraction; // (pixel)

    //
    const auto sceneRect
            = contentsRectInScene.marginsAdded(QMarginsF(marginX, marginY, marginX, marginY));
    graphicsView->setSceneRect(sceneRect);
}

void BoardView::doApplyZoomAction(const ZoomAction zoomAction, const QPointF &anchorScenePos) {
    if (zoomAction == ZoomAction::ResetZoom) {
        zoomScale = 1.0;
    }
    else {
        const QVector<double> scaleFactors {
            0.5, 0.67, 0.75, 0.8, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0
        }; // must be non-empty & strictly increasing

        const double oldScale = zoomScale;
        const int oldZoomLevel = findIndexOfClosestValue(scaleFactors, oldScale, true);
        const int zoomLevel = (zoomAction == ZoomAction::ZoomIn)
                ? std::min(oldZoomLevel + 1, scaleFactors.count() - 1)
                : std::max(oldZoomLevel - 1, 0);
        zoomScale = scaleFactors.at(zoomLevel);
    }

    updateCanvasScale(zoomScale * graphicsGeometryScaleFactor, anchorScenePos);
}

void BoardView::updateCanvasScale(const double scale, const QPointF &anchorScenePos) {
    const QPointF anchorPosInCanvas = canvas->mapFromScene(anchorScenePos);
    canvas->setScale(scale);

    // move the scene
    const QPointF driftedAnchorPosInScene = canvas->mapToScene(anchorPosInCanvas);
    const auto displacementToApply = anchorScenePos - driftedAnchorPosInScene;
    if (vectorLength(displacementToApply) > 1e-3)
        moveSceneRelativeToView(displacementToApply);

    //
    adjustSceneRect();
}

void BoardView::updatePropertiesDisplayOfAllCards() {
    // merge (cascade) workspace's setting with board's setting
    CardPropertiesToShow effectiveSetting = cardPropertiesToShowSettings.onWorkspace;
    effectiveSetting.updateWith(cardPropertiesToShowSettings.onBoard);

    //
    const QSet<int> cardIds = nodeRectsCollection.getAllCardIds();

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QHash<int, Card> cards;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, cardIds]() {
        // get cards data
        Services::instance()->getAppDataReadonly()->queryCards(
                cardIds,
                // callback
                [routine](bool ok, const QHash<int, Card> &cards) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get cards data.";
                        return;
                    }
                    routine->cards = cards;
                },
                this);
    }, this);

    routine->addStep([this, routine, effectiveSetting]() {
        // apply `effectiveSetting` on every card
        ContinuationContext context(routine);

        for (auto it = routine->cards.constBegin(); it != routine->cards.constEnd(); ++it) {
            const int &cardId = it.key();
            const Card &cardData = it.value();
            nodeRectsCollection.updateNodeRectPropertiesDisplay(
                    cardId, cardData.getLabels(), cardData.getCustomProperties(),effectiveSetting);
        }

    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void BoardView::updateRelationshipBundles() {
    // show all EdgeArrow's of relationships
    relationshipsCollection.setAllEdgeArrowsVisible();

    // update bundles
    relationshipBundlesCollection.update();

    // hide EdgeArrow's of bundled relationships
    const auto rels = relationshipBundlesCollection.getBundledRelationships();
    relationshipsCollection.hideEdgeArrows(rels);
}

void BoardView::moveFollowerItemsInComovingState(
        const QPointF &displacement, const ComovingStateData &comovingStateData) {
    if (!comovingStateData.getIsActive())
        return;

    // follower group-boxes
    const auto followerGroupBoxIdToInitialPos
            = comovingStateData.followerGroupBoxIdToInitialPos;
    for (auto it = followerGroupBoxIdToInitialPos.constBegin();
            it != followerGroupBoxIdToInitialPos.constEnd(); ++it) {
        const int groupBoxId = it.key();
        GroupBox *groupBox = groupBoxesCollection.get(groupBoxId);
        if (groupBox != nullptr) {
            QRectF rect = groupBox->getRect();
            rect.moveTopLeft(it.value() + displacement);
            groupBox->setRect(rect);
        }

        // relationship bundles
        relationshipBundlesCollection.updateBundlesConnectingGroupBox(groupBoxId);
    }

    // follower NodeRect's
    const auto followerCardIdToInitialPos
            = comovingStateData.followerCardIdToInitialPos;
    for (auto it = followerCardIdToInitialPos.constBegin();
            it != followerCardIdToInitialPos.constEnd(); ++it) {
        const int cardId = it.key();
        NodeRect *nodeRect = nodeRectsCollection.get(cardId);
        if (nodeRect != nullptr) {
            QRectF rect = nodeRect->getRect();
            rect.moveTopLeft(it.value() + displacement);
            nodeRect->setRect(rect);
        }

        // relationship bundles
        relationshipBundlesCollection.updateBundlesConnectingNodeRect(cardId);
    }

    // EdgeArrow's
    QSet<RelationshipId> affectedRelIds;
    for (auto it = followerCardIdToInitialPos.constBegin();
            it != followerCardIdToInitialPos.constEnd(); ++it) {
        const int cardId = it.key();
        affectedRelIds += getEdgeArrowsConnectingNodeRect(cardId);
    }

    for (const auto &relId: qAsConst(affectedRelIds)) {
        constexpr bool updateOtherEdgeArrows = false;
        relationshipsCollection.updateEdgeArrow(relId, updateOtherEdgeArrows);
    }
}

void BoardView::savePositionsOfComovingItems(const ComovingStateData &comovingStateData) {
    const auto groupBoxIds = keySet(comovingStateData.followerGroupBoxIdToInitialPos);
    const auto cardIds = keySet(comovingStateData.followerCardIdToInitialPos);

    for (const int groupBoxId: groupBoxIds) {
        GroupBox *groupBox = groupBoxesCollection.get(groupBoxId);
        if (groupBox == nullptr)
            return;

        GroupBoxNodePropertiesUpdate update;
        update.rect = groupBox->getRect();

        Services::instance()->getAppData()->updateGroupBoxProperties(
                EventSource(this), groupBoxId, update);
    }

    for (const int cardId: cardIds) {
        const auto nodeRectRectOpt = nodeRectsCollection.getNodeRectRect(cardId);
        if (!nodeRectRectOpt.has_value())
            return;

        NodeRectDataUpdate update;
        update.rect = nodeRectRectOpt.value();

        Services::instance()->getAppData()->updateNodeRectProperties(
                EventSource(this), this->boardId, cardId, update);
    }
}

void BoardView::getWorkspaceId(std::function<void (const int workspaceId)> callback) {
    Services::instance()->getAppDataReadonly()->getWorkspaces(
            // callback
            [this, callback](bool ok, const QHash<int, Workspace> &workspaces) {
                if (!ok) {
                    callback(-1);
                    return;
                }

                // find workspace containing this->boardId
                int workspaceId = -1;
                for (auto it = workspaces.constBegin(); it != workspaces.constEnd(); ++it) {
                    if (it.value().boardIds.contains(boardId)) {
                        workspaceId = it.key();
                        break;
                    }
                }

                callback(workspaceId);
            },
            this
    );
}

QRectF BoardView::getContentsRectInCanvasCoordinates() const {
    const QRectF boundingRect1 = nodeRectsCollection.getBoundingRectOfAllNodeRects();
    const QRectF boundingRect2 = relationshipsCollection.getBoundingRectOfAllEdgeArrows();
    const QRectF boundingRect3 = dataViewBoxesCollection.getBoundingRectOfAllDataViewBoxes();
    const QRectF boundingRect4 = groupBoxesCollection.getBoundingRectOfAllGroupBoxes();
    const QRectF boundingRect5 = relationshipBundlesCollection.getBoundingRectOfAllArrows();
    const QRectF boundingRect6 = settingBoxesCollection.getBoundingRectOfAllSettingBoxes();

    return boundingRectOfRects(
            { boundingRect1, boundingRect2, boundingRect3,
              boundingRect4, boundingRect5, boundingRect6 }
    );
}

QColor BoardView::getSceneBackgroundColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(darkThemeBoardBackground) : QColor(230, 230, 230);
}

QColor BoardView::getEdgeArrowLineColor() const {
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    return isDarkTheme ? QColor(175, 175, 175) : QColor(100, 100, 100);
}

QColor BoardView::getEdgeArrowLabelColor() const {
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    return isDarkTheme ? QColor(darkThemeStandardTextColor) : QColor(Qt::black);
}

QColor BoardView::computeGroupBoxColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(75, 79, 83) : QColor(170, 170, 170);
}

QPoint BoardView::getScreenPosFromScenePos(const QPointF &scenePos) const {
    QPoint posInViewport = graphicsView->mapFromScene(scenePos);
    return graphicsView->viewport()->mapToGlobal(posInViewport);
}

QPointF BoardView::getViewCenterInScene() const {
    return graphicsView->mapToScene(
            graphicsView->viewport()->width() / 2,
            graphicsView->viewport()->height() / 2);
}

void BoardView::setViewTopLeftPos(const QPointF &canvasPos) {
    const auto scenePos = canvas->mapToScene(canvasPos);
    const double centerX = scenePos.x() + graphicsView->viewport()->width() * 0.5;
    const double centerY = scenePos.y() + graphicsView->viewport()->height() * 0.5;
    graphicsView->centerOn(centerX, centerY);
}

void BoardView::moveSceneRelativeToView(const QPointF &displacement) {
    const auto viewCenter = getViewCenterInScene();
    const auto newViewCenter = viewCenter - displacement;
    graphicsView->centerOn(newViewCenter);
}

QColor BoardView::computeNodeRectDisplayColor(
        const QColor &nodeRectOwnColor, const QSet<QString> &cardLabels,
        const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &boardDefaultColorForNodeRect, const bool invertLightness) {
    std::function<QColor ()> funcGetColor1 = [&]() {
        // 1. NodeRect's own color
        if (nodeRectOwnColor.isValid())
            return nodeRectOwnColor;

        // 2. card labels & board's `cardLabelsAndAssociatedColors`
        for (const auto &labelAndColor: cardLabelsAndAssociatedColors) {
            if (cardLabels.contains(labelAndColor.first))
                return labelAndColor.second;
        }

        // 3. board's default
        if (boardDefaultColorForNodeRect.isValid())
            return boardDefaultColorForNodeRect;

        //
        return QColor(170, 170, 170);
    };

    QColor color = funcGetColor1();

    if (invertLightness)
        color = invertHslLightness(color);

    //
    return color;
}

QColor BoardView::computeDataViewBoxDisplayColor(
        const QColor &dataViewBoxOwnColor, const QColor &boardDefaultColorForDataViewBox) {
    if (dataViewBoxOwnColor.isValid())
        return dataViewBoxOwnColor;

    if (boardDefaultColorForDataViewBox.isValid())
        return boardDefaultColorForDataViewBox;

    return QColor(170, 170, 170);
}

QColor BoardView::computeSettingBoxDisplayColor(
        const QColor &boardDefaultColorForSettingBox, const bool invertLightness) {
    QColor color = boardDefaultColorForSettingBox;

    if (invertLightness)
        color = invertHslLightness(color);

    return color;
}

QSet<RelationshipId> BoardView::getEdgeArrowsConnectingNodeRect(const int cardId) {
    QSet<RelationshipId> result;

    const auto relIds = relationshipsCollection.getAllRelationshipIds();
    for (const RelationshipId &relId: relIds) {
        if (relId.connectsCard(cardId))
            result << relId;
    }

    return result;
}

QLineF BoardView::computeArrowLineConnectingRects(
        const QRectF &fromRect, const QRectF &toRect,
        const int parallelIndex, const int parallelCount) {
    // Compute the vector for translating the center-to-center line. The result must be invariant
    // if `fromRect` and `toRect` are swapped.
    QPointF vecTranslation;
    {
        constexpr double spacing = 22;
        const double shiftDistance = (parallelIndex - (parallelCount - 1.0) / 2.0) * spacing;

        //
        QPointF p1 = fromRect.center();
        QPointF p2 = toRect.center();

        const int x1 = nearestInteger(p1.x() * 100);
        const int x2 = nearestInteger(p2.x() * 100);
        if (x1 > x2) {
            std::swap(p1, p2);
        }
        else if (x1 == x2) {
            const int y1 = nearestInteger(p1.y() * 100);
            const int y2 = nearestInteger(p2.y() * 100);
            if (y1 > y2)
                std::swap(p1, p2);
        }

        const QLineF lineNormal = QLineF(p1, p2).normalVector().unitVector();
        const QPointF vecNormal(lineNormal.dx(), lineNormal.dy());
        vecTranslation = vecNormal * shiftDistance;
    }

    //
    const QLineF lineC2CTranslated
            = QLineF(fromRect.center(), toRect.center())
              .translated(vecTranslation);

    //
    bool intersect;
    QPointF intersectionPoint;

    intersect = rectEdgeIntersectsWithLine(fromRect, lineC2CTranslated, &intersectionPoint);
    const QPointF startPoint = intersect ?  intersectionPoint : fromRect.center();

    intersect = rectEdgeIntersectsWithLine(toRect, lineC2CTranslated, &intersectionPoint);
    const QPointF endPoint = intersect ? intersectionPoint : toRect.center();

    return {startPoint, endPoint};
}

QString BoardView::computeCardPropertiesDisplay(
        CardPropertiesToShow effectiveCardPropertiesToShowSetting, const QSet<QString> &cardLabels,
        const QHash<QString, QJsonValue> &cardCustomProperties) {
    // 1. determine properties display format by labels
    const CardPropertiesToShow::PropertiesAndDisplayFormats propertiesDisplayFormat
            = effectiveCardPropertiesToShowSetting.getPropertiesToShow(cardLabels);

    // 2. determine properties display text
    QStringList displayOfProperties;
    for (const auto &[propertyName, format]: propertiesDisplayFormat) {
        QString valueDisplay;
        if (!cardCustomProperties.contains(propertyName)) {
            if (format.stringIfNotExists.has_value())
                valueDisplay = format.stringIfNotExists.value();
            else
                continue;
        }
        else {
            const QJsonValue value = cardCustomProperties.value(propertyName);
            valueDisplay = format.getValueDisplayText(value);
        }

        //
        QString propertyDisplay;
        if (!format.hideLabel)
            propertyDisplay = QString("%1: %2").arg(propertyName, valueDisplay);
        else
            propertyDisplay = valueDisplay;

        //
        displayOfProperties << propertyDisplay;
    }

    return displayOfProperties.join("\n");
}

//======

NodeRect *BoardView::NodeRectsCollection::createNodeRect(
        const int cardId, const Card &cardData, const QRectF &rect,
        const QColor &displayColor, const QColor &nodeRectOwnColor,
        const QStringList &userLabelsList, const QString &propertiesDisplay) {
    Q_ASSERT(!cardIdToNodeRect.contains(cardId));

    auto *nodeRect = new NodeRect(cardId, boardView->canvas);
    cardIdToNodeRect.insert(cardId, nodeRect);
    cardIdToNodeRectOwnColor.insert(cardId, nodeRectOwnColor);
    nodeRect->setZValue(zValueForNodeRects);
    nodeRect->initialize();

    const QVector<QString> nodeLabelsVec
            = sortByOrdering(cardData.getLabels(), userLabelsList, false);
    nodeRect->setTitle(cardData.title);
    nodeRect->setText(cardData.text);
    nodeRect->setNodeLabels(QStringList(nodeLabelsVec.cbegin(), nodeLabelsVec.cend()));
    nodeRect->setRect(rect);
    nodeRect->setColor(displayColor);
    nodeRect->setPropertiesDisplay(propertiesDisplay);

    // set up connections
    QPointer<NodeRect> nodeRectPtr(nodeRect);

    QObject::connect(
            nodeRect, &NodeRect::leftButtonPressedOrClicked, boardView, [this, nodeRectPtr]() {
        if (nodeRectPtr.isNull())
            return;

        setHighlightedCardIds({nodeRectPtr->getCardId()});
        boardView->groupBoxesCollection.setHighlightedGroupBoxes({});

        // call AppData
        Services::instance()->getAppData()
                ->setSingleHighlightedCardId(EventSource(boardView), nodeRectPtr->getCardId());
    });

    QObject::connect(nodeRect, &NodeRect::aboutToMove, boardView, [this, cardId, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        boardView->itemMovingResizingStateData.activateWithTargetNodeRect(cardId);
    });

    QObject::connect(nodeRect, &NodeRect::aboutToResize, boardView, [this, cardId, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        boardView->itemMovingResizingStateData.activateWithTargetNodeRect(cardId);
    });

    QObject::connect(
            nodeRect, &NodeRect::movedOrResized, boardView, [this, cardId, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        boardView->relationshipBundlesCollection.updateBundlesConnectingNodeRect(cardId);

        // update edge arrows
        const QSet<RelationshipId> relIds = boardView->getEdgeArrowsConnectingNodeRect(cardId);
        for (const auto &relId: relIds) {
            constexpr bool updateOtherEdgeArrows = false;
            boardView->relationshipsCollection.updateEdgeArrow(relId, updateOtherEdgeArrows);
        }

        // can be added to a group-box?
        {
            const std::optional<int> groupBoxIdOpt
                    = boardView->groupBoxesCollection.getDeepestEnclosingGroupBox(
                            nodeRectPtr.data());

            // -- highlight only the group-box
            boardView->groupBoxesCollection.setHighlightedGroupBoxes(
                    groupBoxIdOpt.has_value() ? QSet<int> {groupBoxIdOpt.value()} : QSet<int> {});

            // -- set new parent to be applied when moving/resizing finishes
            boardView->itemMovingResizingStateData.newParentGroupBoxId = groupBoxIdOpt.value_or(-1);
        }
    });

    QObject::connect(nodeRect, &NodeRect::finishedMovingOrResizing,
                     boardView, [this, cardId, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        //
        boardView->adjustSceneRect();

        // call AppData -- NodeRect properties
        NodeRectDataUpdate update;
        update.rect = nodeRectPtr->getRect();

        Services::instance()->getAppData()->updateNodeRectProperties(
                EventSource(boardView),
                boardView->boardId, nodeRectPtr->getCardId(), update);

        // deactivate `itemMovingResizingStateData`, after necessary actions
        if (boardView->itemMovingResizingStateData.targetIsNodeRect(cardId)) { // should be true
            // action: reparent/add the NodeRect to new parent (or remove it from original parent)
            if (boardView->itemMovingResizingStateData.newParentGroupBoxId.has_value()) {
                const int newParentGroupBox
                        = boardView->itemMovingResizingStateData.newParentGroupBoxId.value(); // can be -1
                boardView->reparentNodeRectInGroupBoxTree(cardId, newParentGroupBox);
            }
        }
        boardView->itemMovingResizingStateData.deactivate();
    });

    QObject::connect(nodeRect, &NodeRect::titleTextUpdated,
            boardView,
            [this, nodeRectPtr](
                    const std::optional<QString> &updatedTitle,
                    const std::optional<QString> &updatedText) {
        if (!nodeRectPtr)
            return;

        // call AppData
        CardPropertiesUpdate propertiesUpdate;
        {
            propertiesUpdate.title = updatedTitle;
            propertiesUpdate.text = updatedText;
        }
        Services::instance()->getAppData()->updateCardProperties(
                EventSource(boardView), nodeRectPtr->getCardId(), propertiesUpdate);
    });

    QObject::connect(nodeRect, &NodeRect::userToSetLabels, boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        boardView->onUserToSetLabels(nodeRectPtr->getCardId());
    });

    QObject::connect(nodeRect, &NodeRect::userToCreateRelationship,
                     boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        boardView->onUserToCreateRelationship(nodeRectPtr->getCardId());
    });

    QObject::connect(nodeRect, &NodeRect::closeByUser, boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        boardView->onUserToCloseNodeRect(nodeRectPtr->getCardId());
    });

    //
    return nodeRect;
}

void BoardView::NodeRectsCollection::closeNodeRect(
        const int cardId, const bool removeConnectedEdgeArrows,
        bool *highlightedCardIdUpdated) {
    *highlightedCardIdUpdated = false;

    NodeRect *nodeRect = cardIdToNodeRect.take(cardId);
    if (nodeRect == nullptr)
        return;
    cardIdToNodeRectOwnColor.remove(cardId);

    //
    boardView->graphicsScene->removeItem(nodeRect);
    nodeRect->deleteLater();

    if (removeConnectedEdgeArrows) {
        const auto relIds = boardView->getEdgeArrowsConnectingNodeRect(cardId);
        boardView->relationshipsCollection.removeEdgeArrows(relIds);
    }

    //
    boardView->graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // This is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug

    //
    if (nodeRect->getIsHighlighted())
        *highlightedCardIdUpdated = true;
}

void BoardView::NodeRectsCollection::updateNodeRectPropertiesDisplay(
        const int cardId, const QSet<QString> &cardLabels,
        const QHash<QString, QJsonValue> &cardCustomProperties,
        const CardPropertiesToShow &effectiveCardPropertiesToShow) {
    if (!cardIdToNodeRect.contains(cardId))
        return;

    const QString propertiesDisplay = computeCardPropertiesDisplay(
            effectiveCardPropertiesToShow, cardLabels, cardCustomProperties);
    cardIdToNodeRect.value(cardId)->setPropertiesDisplay(propertiesDisplay);
}

void BoardView::NodeRectsCollection::setHighlightedCardIds(const QSet<int> &cardIdsToHighlight) {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        it.value()->setIsHighlighted(cardIdsToHighlight.contains(it.key()));
}

QSet<int> BoardView::NodeRectsCollection::addToHighlightedCards(const QSet<int> &cardIdsToHighlight) {
    QSet<int> cardsInHighlightedState;
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        const int cardId = it.key();
        if (cardIdsToHighlight.contains(cardId)) {
            it.value()->setIsHighlighted(true);
            cardsInHighlightedState << cardId;
        }
        else {
            if (it.value()->getIsHighlighted())
                cardsInHighlightedState << cardId;
        }
    }
    return cardsInHighlightedState;
}

void BoardView::NodeRectsCollection::updateAllNodeRectColors() {
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const bool autoAdjustCardColorsForDarkTheme
            = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();

    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        const int cardId = it.key();
        NodeRect *nodeRect = it.value();

        const QColor color = computeNodeRectDisplayColor(
                cardIdToNodeRectOwnColor.value(cardId, QColor()),
                nodeRect->getNodeLabels(),
                boardView->cardLabelsAndAssociatedColors, boardView->defaultNodeRectColor,
                autoAdjustCardColorsForDarkTheme && isDarkTheme);
        nodeRect->setColor(color);
    }
}

void BoardView::NodeRectsCollection::setAllNodeRectsTextEditorIgnoreWheelEvent(const bool b) {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        it.value()->setTextEditorIgnoreWheelEvent(b);
}

bool BoardView::NodeRectsCollection::contains(const int cardId) const {
    return cardIdToNodeRect.contains(cardId);
}

NodeRect *BoardView::NodeRectsCollection::get(const int cardId) const {
    return cardIdToNodeRect.value(cardId);
}

std::optional<QRectF> BoardView::NodeRectsCollection::getNodeRectRect(const int cardId) const {
    if (!cardIdToNodeRect.contains(cardId))
        return std::nullopt;
    return cardIdToNodeRect.value(cardId)->getRect();
}

QSet<int> BoardView::NodeRectsCollection::getAllCardIds() const {
    return keySet(cardIdToNodeRect);
}

QHash<int, QSet<QString> > BoardView::NodeRectsCollection::getCardIdToLabels() const {
    QHash<int, QSet<QString> > cardIdToLabels;
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        cardIdToLabels.insert(it.key(), it.value()->getNodeLabels());
    return cardIdToLabels;
}

QColor BoardView::NodeRectsCollection::getNodeRectOwnColor(const int cardId) const {
    return cardIdToNodeRectOwnColor.value(cardId, QColor());
}

QRectF BoardView::NodeRectsCollection::getBoundingRectOfAllNodeRects() const {
    QRectF result;
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        if (result.isNull())
            result = it.value()->boundingRect();
        else
            result = result.united(it.value()->boundingRect());
    }
    return result;
}

//====

EdgeArrow *BoardView::RelationshipsCollection::createEdgeArrow(
        const RelationshipId relId, const EdgeArrowData &edgeArrowData) {
    Q_ASSERT(!relIdToEdgeArrow.contains(relId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.startCardId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.endCardId));

    auto *edgeArrow = new EdgeArrow(boardView->canvas);
    relIdToEdgeArrow.insert(relId, edgeArrow);
    cardIdPairToParallelRels[QSet<int> {relId.startCardId, relId.endCardId}] << relId;

    edgeArrow->setZValue(zValueForEdgeArrows);

    edgeArrow->setLineWidth(edgeArrowData.lineWidth);
    edgeArrow->setLineColor(edgeArrowData.lineColor);
    edgeArrow->setLabelColor(edgeArrowData.labelColor);

    edgeArrow->setAllowAddingJoints(true);
    edgeArrow->setJoints(edgeArrowData.joints);

    constexpr bool updateOtherEdgeArrows = true;
    updateEdgeArrow(relId, updateOtherEdgeArrows);

    // connections
    QPointer<EdgeArrow> edgeArrowPtr(edgeArrow);

    connect(edgeArrow, &EdgeArrow::finishedUpdatingJoints,
            boardView, [this, edgeArrowPtr, relId](const QVector<QPointF> &/*joints*/) {
        if (edgeArrowPtr.isNull())
            return;

        //
        constexpr bool updateOtherEdgeArrows = true;
        updateEdgeArrow(relId, updateOtherEdgeArrows);

        // call AppData
        BoardNodePropertiesUpdate update;
        update.relIdToJoints = getRelIdToJoints();

        Services::instance()->getAppData()->updateBoardNodeProperties(
                EventSource(boardView), boardView->boardId, update);
    });

    connect(edgeArrow, &EdgeArrow::jointMoved, boardView, [this, edgeArrowPtr, relId]() {
        if (edgeArrowPtr.isNull())
            return;

        Q_ASSERT(!edgeArrowPtr->getJoints().isEmpty());
        updateSingleEdgeArrow(relId, -1, 0);
    });

    //
    return edgeArrow;
}

void BoardView::RelationshipsCollection::updateEdgeArrow(
        const RelationshipId &relId, const bool updateOtherEdgeArrows) {
    Q_ASSERT(relIdToEdgeArrow.contains(relId));

    const QSet<RelationshipId> parallelRelIds // all relationships connecting the 2 cards
            = cardIdPairToParallelRels.value(QSet<int> {relId.startCardId, relId.endCardId});
    Q_ASSERT(parallelRelIds.contains(relId));

    QSet<RelationshipId> parallelRelsWithoutJoint;
    for (const RelationshipId &relId: parallelRelIds) {
        if (relIdToEdgeArrow.value(relId)->getJoints().isEmpty())
            parallelRelsWithoutJoint << relId;
    }

    const QVector<RelationshipId> sortedParallelRelsWithoutJoint
            = sortRelationshipIds(parallelRelsWithoutJoint);

    //
    QSet<RelationshipId> relsToUpdate = updateOtherEdgeArrows
            ? parallelRelIds : QSet<RelationshipId> {relId};
    for (const RelationshipId &relId1: relsToUpdate) {
        const int index = sortedParallelRelsWithoutJoint.indexOf(relId1); // can be -1
        updateSingleEdgeArrow(relId1, index, sortedParallelRelsWithoutJoint.count());
    }
}

void BoardView::RelationshipsCollection::removeEdgeArrows(const QSet<RelationshipId> &relIds) {
    for (const auto &relId: relIds) {
        if (!relIdToEdgeArrow.contains(relId))
            continue;

        EdgeArrow *edgeArrow = relIdToEdgeArrow.take(relId);

        boardView->graphicsScene->removeItem(edgeArrow);
        delete edgeArrow;

        //
        const QSet<int> cardIdPair {relId.startCardId, relId.endCardId};
        cardIdPairToParallelRels[cardIdPair].remove(relId);
        if (cardIdPairToParallelRels[cardIdPair].isEmpty())
            cardIdPairToParallelRels.remove(cardIdPair);
    }
}

void BoardView::RelationshipsCollection::setAllEdgeArrowsVisible() {
    for (auto it = relIdToEdgeArrow.constBegin(); it != relIdToEdgeArrow.constEnd(); ++it)
        it.value()->setVisible(true);
}

void BoardView::RelationshipsCollection::setLineColorAndLabelColorOfAllEdgeArrows(
        const QColor &lineColor, const QColor &labelColor) {
    for (auto it = relIdToEdgeArrow.constBegin(); it != relIdToEdgeArrow.constEnd(); ++it) {
        it.value()->setLineColor(lineColor);
        it.value()->setLabelColor(labelColor);
    }
}

void BoardView::RelationshipsCollection::hideEdgeArrows(const QSet<RelationshipId> &relIds) {
    for (const auto &rel: relIds) {
        EdgeArrow *edgeArrow = relIdToEdgeArrow.value(rel);
        if (edgeArrow != nullptr)
            edgeArrow->setVisible(false);
    }
}

QSet<RelationshipId> BoardView::RelationshipsCollection::getAllRelationshipIds() const {
    return keySet(relIdToEdgeArrow);
}

QHash<RelationshipId, QVector<QPointF>>
BoardView::RelationshipsCollection::getRelIdToJoints() const {
    QHash<RelationshipId, QVector<QPointF>> relIdToJoints;
    for (auto it = relIdToEdgeArrow.constBegin(); it != relIdToEdgeArrow.constEnd(); ++it) {
        const auto joints = it.value()->getJoints();
        if (!joints.isEmpty())
            relIdToJoints.insert(it.key(), joints);
    }
    return relIdToJoints;
}

QRectF BoardView::RelationshipsCollection::getBoundingRectOfAllEdgeArrows() const {
    QRectF result;
    for (auto it = relIdToEdgeArrow.constBegin(); it != relIdToEdgeArrow.constEnd(); ++it) {
        if (result.isNull())
            result = it.value()->boundingRect();
        else
            result = result.united(it.value()->boundingRect());
    }
    return result;
}

//!
//! Updates endpoints and label of the EdgeArrow for \e relId.
//! \param relId
//! \param parallelIndex: index of \e relId in the sorted list of all parallel relationships
//!             without joint. -1 means \e relId has joint.
//! \param countOfParallelRelsWithoutJoint: number of all parallel relationships that have no
//!             joint. Used only if \e relId has no joint.
//!
void BoardView::RelationshipsCollection::updateSingleEdgeArrow(
        const RelationshipId &relId,
        const int parallelIndex, const int countOfParallelRelsWithoutJoint) {
    if (parallelIndex != -1) {
        Q_ASSERT(countOfParallelRelsWithoutJoint >= 1);
        Q_ASSERT(parallelIndex < countOfParallelRelsWithoutJoint);
    }

    auto *edgeArrow = relIdToEdgeArrow.value(relId);
    const auto joints = edgeArrow->getJoints();

    //
    if (joints.isEmpty()) {
        Q_ASSERT (parallelIndex != -1);
        const QLineF line = computeEdgeArrowLineWithoutJoint(
                relId, parallelIndex, countOfParallelRelsWithoutJoint);
        edgeArrow->setStartEndPoint(line.p1(), line.p2());
    }
    else {
        const auto [startPoint, endPoint]
                = computeEndpointsOfEdgeArrowWithJoint(relId, joints.first(), joints.last());
        edgeArrow->setStartEndPoint(startPoint, endPoint);
    }

    //
    edgeArrow->setLabel(relId.type);
}

QVector<RelationshipId> BoardView::RelationshipsCollection::sortRelationshipIds(
        const QSet<RelationshipId> relIds) {
    QVector<RelationshipId> result(relIds.constBegin(), relIds.constEnd());
    std::sort(
            result.begin(), result.end(),
            [](const RelationshipId &a, const RelationshipId &b) ->bool {
                return std::tie(a.startCardId, a.endCardId, a.type)
                        < std::tie(b.startCardId, b.endCardId, b.type);
            }
    );
    return result;
}

QLineF BoardView::RelationshipsCollection::computeEdgeArrowLineWithoutJoint(
        const RelationshipId &relId, const int parallelIndex, const int parallelCount) {
    const auto rectOfStartNodeRectOpt
            = boardView->nodeRectsCollection.getNodeRectRect(relId.startCardId);
    Q_ASSERT(rectOfStartNodeRectOpt.has_value());

    const auto rectOfEndNodeRectOpt
            = boardView->nodeRectsCollection.getNodeRectRect(relId.endCardId);
    Q_ASSERT(rectOfEndNodeRectOpt.has_value());

    return computeArrowLineConnectingRects(
            rectOfStartNodeRectOpt.value(), rectOfEndNodeRectOpt.value(),
            parallelIndex, parallelCount);
}

std::pair<QPointF, QPointF>
BoardView::RelationshipsCollection::computeEndpointsOfEdgeArrowWithJoint(
        const RelationshipId &relId, const QPointF &firstJoint, const QPointF &lastJoint) {
    QPointF startPoint;
    {
        const auto rectOfStartNodeRectOpt
                = boardView->nodeRectsCollection.getNodeRectRect(relId.startCardId);
        Q_ASSERT(rectOfStartNodeRectOpt.has_value());

        const QLineF line(rectOfStartNodeRectOpt.value().center(), firstJoint);
        QPointF intersectionPoint;
        const bool intersects = rectEdgeIntersectsWithLine(
                rectOfStartNodeRectOpt.value(), line, &intersectionPoint);
        if (intersects)
            startPoint = intersectionPoint;
        else
            startPoint = rectOfStartNodeRectOpt.value().center();
    }

    QPointF endPoint;
    {
        const auto rectOfEndNodeRectOpt
                = boardView->nodeRectsCollection.getNodeRectRect(relId.endCardId);
        Q_ASSERT(rectOfEndNodeRectOpt.has_value());

        const QLineF line(rectOfEndNodeRectOpt.value().center(), lastJoint);
        QPointF intersectionPoint;
        const bool intersects = rectEdgeIntersectsWithLine(
                rectOfEndNodeRectOpt.value(), line, &intersectionPoint);
        if (intersects)
            endPoint = intersectionPoint;
        else
            endPoint = rectOfEndNodeRectOpt.value().center();
    }

    return {startPoint, endPoint};
}

DataViewBox *BoardView::DataViewBoxesCollection::createDataViewBox(
        const int customDataQueryId, const CustomDataQuery &customDataQueryData,
        const QRectF &rect, const QColor &displayColor, const QColor &dataViewBoxOwnColor) {
    Q_ASSERT(!customDataQueryIdToDataViewBox.contains(customDataQueryId));

    auto *box = new DataViewBox(customDataQueryId, boardView->canvas);
    customDataQueryIdToDataViewBox.insert(customDataQueryId, box);
    customDataQueryIdToDataViewBoxOwnColor.insert(customDataQueryId, dataViewBoxOwnColor);
    box->setZValue(zValueForNodeRects);
    box->initialize();

    box->setTitle(customDataQueryData.title);
    box->setQuery(customDataQueryData.queryCypher, customDataQueryData.queryParameters);
    box->setRect(rect);
    box->setColor(displayColor);

    // set up connections
    QPointer<DataViewBox> boxPtr(box);

    QObject::connect(
            box, &DataViewBox::getCardIdsOfBoard, boardView, [this](QSet<int> *cardIds) {
        *cardIds = boardView->nodeRectsCollection.getAllCardIds();
    }, Qt::DirectConnection);

    QObject::connect(
            box, &DataViewBox::titleUpdated,
            boardView, [this, boxPtr](const QString &updatedTitle) {
        if (!boxPtr)
            return;

        // call AppData
        CustomDataQueryUpdate update;
        {
            update.title = updatedTitle;
        }
        Services::instance()->getAppData()->updateCustomDataQueryProperties(
                EventSource(boardView), boxPtr->getCustomDataQueryId(), update);
    });

    QObject::connect(
            box, &DataViewBox::queryUpdated,
            boardView, [this, boxPtr](const QString &cypher, const QJsonObject &parameters) {
        if (!boxPtr)
            return;

        // call AppData
        CustomDataQueryUpdate update;
        {
            update.queryCypher = cypher;
            update.queryParameters = parameters;
        }
        Services::instance()->getAppData()->updateCustomDataQueryProperties(
                EventSource(boardView), boxPtr->getCustomDataQueryId(), update);
    });

    QObject::connect(box, &DataViewBox::finishedMovingOrResizing, boardView, [this, boxPtr]() {
        if (!boxPtr)
            return;

        //
        boardView->adjustSceneRect();

        // call AppData
        DataViewBoxDataUpdate update;
        update.rect = boxPtr->getRect();

        Services::instance()->getAppData()->updateDataViewBoxProperties(
                EventSource(boardView),
                boardView->boardId, boxPtr->getCustomDataQueryId(), update);
    });

    QObject::connect(box, &DataViewBox::closeByUser, boardView, [this, boxPtr]() {
        if (!boxPtr)
            return;
        boardView->onUserToCloseDataViewBox(boxPtr->getCustomDataQueryId());
    });

    //
    return box;
}

void BoardView::DataViewBoxesCollection::closeDataViewBox(const int customDataQueryId) {
    DataViewBox *box = customDataQueryIdToDataViewBox.take(customDataQueryId);
    if (box == nullptr)
        return;
    customDataQueryIdToDataViewBoxOwnColor.remove(customDataQueryId);

    //
    boardView->graphicsScene->removeItem(box);
    box->deleteLater();

    //
    boardView->graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // This is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug
}

void BoardView::DataViewBoxesCollection::setAllDataViewBoxesTextEditorIgnoreWheelEvent(
        const bool ignoreWheelEvent) {
    for (auto it = customDataQueryIdToDataViewBox.begin();
            it != customDataQueryIdToDataViewBox.end(); ++it) {
         it.value()->setTextEditorIgnoreWheelEvent(ignoreWheelEvent);
    }
}

bool BoardView::DataViewBoxesCollection::contains(const int customDataQueryId) const {
    return customDataQueryIdToDataViewBox.contains(customDataQueryId);
}

QSet<int> BoardView::DataViewBoxesCollection::getAllCustomDataQueryIds() const {
    return keySet(customDataQueryIdToDataViewBox);
}

QRectF BoardView::DataViewBoxesCollection::getBoundingRectOfAllDataViewBoxes() const {
    QRectF result;
    for (auto it = customDataQueryIdToDataViewBox.constBegin();
            it != customDataQueryIdToDataViewBox.constEnd(); ++it) {
        if (result.isNull())
            result = it.value()->boundingRect();
        else
            result = result.united(it.value()->boundingRect());
    }
    return result;
}

//====

GroupBox *BoardView::GroupBoxesCollection::createGroupBox(
        const int groupBoxId, const GroupBoxData &groupBoxData) {
    Q_ASSERT(groupBoxId != -1);
    Q_ASSERT(!groupBoxes.contains(groupBoxId));

    auto *groupBox = new GroupBox(groupBoxId, boardView->canvas);
    groupBoxes.insert(groupBoxId, groupBox);
    groupBox->setZValue(zValueForNodeRects);
    groupBox->initialize();

    groupBox->setTitle(groupBoxData.title);
    groupBox->setRect(groupBoxData.rect);
    groupBox->setBorderWidth(3);

    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    groupBox->setColor(computeGroupBoxColor(isDarkTheme));

    // set up connections
    QPointer<GroupBox> groupBoxPtr(groupBox);

    QObject::connect(
            groupBox, &GroupBox::leftButtonPressed, boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        constexpr bool unhighlightOtherItems = true;
        highlightGroupBoxAndDescendants(groupBoxId, unhighlightOtherItems);
    });

    QObject::connect(
            groupBox, &GroupBox::aboutToMove, boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        // enter moving/resizing state
        const auto [descendantGroupBoxes, descendantCards]
                = boardView->groupBoxTree.getAllDescendants(groupBoxId);

        boardView->itemMovingResizingStateData.activateWithTargetGroupBox(
                groupBoxId, descendantGroupBoxes, descendantCards);

        // enter co-moving state, let all descendants follow the move
        boardView->comovingStateData.activate();
        boardView->comovingStateData.clearFollowers();
        for (const int groupBoxId: descendantGroupBoxes) {
            GroupBox *groupBox = this->groupBoxes.value(groupBoxId);
            if (groupBox != nullptr) {
                boardView->comovingStateData.followerGroupBoxIdToInitialPos.insert(
                        groupBoxId, groupBox->getRect().topLeft());
            }
        }
        for (const int cardId: descendantCards) {
            const auto nodeRectRectOpt = boardView->nodeRectsCollection.getNodeRectRect(cardId);
            if (nodeRectRectOpt.has_value()) {
                boardView->comovingStateData.followerCardIdToInitialPos.insert(
                        cardId, nodeRectRectOpt.value().topLeft());
            }
        }
        boardView->comovingStateData.followeeInitialPos = groupBoxPtr->getRect().topLeft();
    });

    QObject::connect(
            groupBox, &GroupBox::aboutToResize,
            boardView, [this, groupBoxId, groupBoxPtr](QRectF *mustKeepEnclosingRect) {
        if (!groupBoxPtr)
            return;

        // determine `*mustKeepEnclosingRect` (the group-box must keep enclosing the bounding
        // rect of its descendant items)
        const auto [descendantGroupBoxes, descendantCards]
                = boardView->groupBoxTree.getAllDescendants(groupBoxId);

        QRectF mustEncloseRect;
        if (!descendantGroupBoxes.isEmpty() || !descendantCards.isEmpty()) {
            QVector<QRectF> descendantItemsRects;
            for (const int groupBoxId: descendantGroupBoxes) {
                if (auto *groupBox = groupBoxes.value(groupBoxId); groupBox != nullptr)
                    descendantItemsRects << groupBox->getRect();
            }
            for (const int cardId: descendantCards) {
                const auto nodeRectRectOpt
                        = boardView->nodeRectsCollection.getNodeRectRect(cardId);
                if (nodeRectRectOpt.has_value())
                    descendantItemsRects << nodeRectRectOpt.value();
            }

            const QRectF boundingRectOfChildItems = boundingRectOfRects(descendantItemsRects);
            const QMarginsF margins
                    = diffMargins(groupBoxPtr->getRect(), groupBoxPtr->getContentsRect());
            mustEncloseRect = boundingRectOfChildItems.marginsAdded(margins);
        }
        *mustKeepEnclosingRect = mustEncloseRect;

        // enter moving/resizing state
        boardView->itemMovingResizingStateData.activateWithTargetGroupBox(
                groupBoxId, descendantGroupBoxes, descendantCards);
    }, Qt::DirectConnection);

    QObject::connect(
            groupBox, &GroupBox::movedOrResized, boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        boardView->relationshipBundlesCollection.updateBundlesConnectingGroupBox(groupBoxId);

        // can be added to a group-box?
        {
            const auto groupBoxesBeingMoved
                    = boardView->itemMovingResizingStateData.descendantGroupBoxesOfTargetGroupBox
                      + QSet<int> {groupBoxId};
            const std::optional<int> addToGroupBoxIdOpt = getDeepestEnclosingGroupBox(
                    groupBoxPtr.data(),
                    groupBoxesBeingMoved // groupBoxIdsToExclude
            );

            // -- highlight `addToGroupBoxIdOpt`, unhighlight other group-boxes except
            //    `groupBoxesBeingMoved`
            if (addToGroupBoxIdOpt.has_value())
                addToHighlightedGroupBoxes({addToGroupBoxIdOpt.value()});

            QSet<int> groupBoxesToUnhighlight = keySet(groupBoxes) - groupBoxesBeingMoved;
            if (addToGroupBoxIdOpt.has_value())
                groupBoxesToUnhighlight.remove(addToGroupBoxIdOpt.value());
            unhighlightGroupBoxes(groupBoxesToUnhighlight);

            // -- set new parent to be applied when moving/resizing finishes
            boardView->itemMovingResizingStateData.newParentGroupBoxId
                    = addToGroupBoxIdOpt.value_or(-1);
        }

        //
        if (boardView->comovingStateData.getIsActive()) {
            const QPointF displacement
                    = groupBoxPtr->getRect().topLeft()
                      - boardView->comovingStateData.followeeInitialPos;
            boardView->moveFollowerItemsInComovingState(displacement, boardView->comovingStateData);
        }
    });

    QObject::connect(
            groupBox, &GroupBox::finishedMovingOrResizing,
            boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        //
        boardView->adjustSceneRect();

        // save properties of `groupBoxId`
        GroupBoxNodePropertiesUpdate update;
        update.rect = groupBoxPtr->getRect();

        Services::instance()->getAppData()->updateGroupBoxProperties(
                EventSource(boardView), groupBoxId, update);

        // save position of co-moving items and deactivate `comovingStateData`
        if (boardView->comovingStateData.getIsActive()) {
            boardView->savePositionsOfComovingItems(boardView->comovingStateData);
            boardView->comovingStateData.deactivate();
        }

        // deactivate `itemMovingResizingStateData`, after necessary actions
        if (boardView->itemMovingResizingStateData.targetIsGroupBox(groupBoxId)) { // should be true
            // action: reparent the group-box
            if (boardView->itemMovingResizingStateData.newParentGroupBoxId.has_value()) {
                const int newParentGroupBox // can be -1
                        = boardView->itemMovingResizingStateData.newParentGroupBoxId.value();
                boardView->reparentGroupBoxInGroupBoxTree(groupBoxId, newParentGroupBox);
            }
        }
        boardView->itemMovingResizingStateData.deactivate();
    });

    QObject::connect(
            groupBox, &GroupBox::userToRenameGroupBox, boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        bool ok;
        const QString newTitle = QInputDialog::getText(
                boardView, "Rename Group Box", "Enter new name:",
                QLineEdit::Normal, groupBoxPtr->getTitle(), &ok);
        if (ok)
            boardView->onUserToSetGroupBoxTitle(groupBoxId, newTitle);
    });

    QObject::connect(
            groupBox, &GroupBox::userToRemoveGroupBox, boardView, [this, groupBoxId, groupBoxPtr]() {
        if (!groupBoxPtr)
            return;

        QString msg = "Remove the grouping?";
        if (boardView->groupBoxTree.hasChild(groupBoxId))
            msg += " (Its contents will not be removed.)";
        const auto r = QMessageBox::question(boardView, " ", msg);
        if (r == QMessageBox::Yes)
            boardView->onUserToRemoveGroupBox(groupBoxId);
    });

    //
    return groupBox;
}

void BoardView::GroupBoxesCollection::removeGroupBox(const int groupBoxId) {
    GroupBox *groupBox = groupBoxes.take(groupBoxId);
    if (groupBox == nullptr)
        return;

    //
    boardView->graphicsScene->removeItem(groupBox);
    groupBox->deleteLater();

    //
    boardView->graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // This is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug
}

void BoardView::GroupBoxesCollection::setHighlightedGroupBoxes(const QSet<int> &groupBoxIds) {
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it)
        it.value()->setIsHighlighted(groupBoxIds.contains(it.key()));
}

void BoardView::GroupBoxesCollection::addToHighlightedGroupBoxes(const QSet<int> &groupBoxIds) {
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it) {
        if (groupBoxIds.contains(it.key()))
            it.value()->setIsHighlighted(true);
    }
}

void BoardView::GroupBoxesCollection::unhighlightGroupBoxes(const QSet<int> &groupBoxIds) {
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it) {
        if (groupBoxIds.contains(it.key()))
            it.value()->setIsHighlighted(false);
    }
}

void BoardView::GroupBoxesCollection::setColorOfAllGroupBoxes(const QColor &color) {
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it)
        it.value()->setColor(color);
}

GroupBox *BoardView::GroupBoxesCollection::get(const int groupBoxId) {
    return groupBoxes.value(groupBoxId);
}

QSet<int> BoardView::GroupBoxesCollection::getAllGroupBoxIds() const {
    return keySet(groupBoxes);
}

QRectF BoardView::GroupBoxesCollection::getBoundingRectOfAllGroupBoxes() const {
    QRectF result;
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it) {
        if (result.isNull())
            result = it.value()->boundingRect();
        else
            result = result.united(it.value()->boundingRect());
    }
    return result;
}

std::optional<int> BoardView::GroupBoxesCollection::getDeepestEnclosingGroupBox(
        const BoardBoxItem *boardBoxItem, const QSet<int> &groupBoxIdsToExclude) {
    const QRectF rect = boardBoxItem->boundingRect();

    // find group-boxes whose contents rects enclose `rect`, excluding `groupBoxIdToExclude`
    QSet<int> enclosingGroupBoxes;
    for (auto it = groupBoxes.constBegin(); it != groupBoxes.constEnd(); ++it) {
        if (groupBoxIdsToExclude.contains(it.key()))
            continue;
        GroupBox *const &groupBox = it.value();
        if (groupBox->getContentsRect().contains(rect))
            enclosingGroupBoxes << it.key();
    }

    if (enclosingGroupBoxes.isEmpty())
        return std::nullopt;

    // if `enclosingGroupBoxes` does not form a single path (i.e., does not consist of single
    // set of nesting group-boxes), return nullopt
    int deepestGroupBox;
    const bool formsSinglePath
            = boardView->groupBoxTree.formsSinglePath(enclosingGroupBoxes, &deepestGroupBox);
    if (!formsSinglePath)
        return std::nullopt;

    // return the deepest group-box
    return deepestGroupBox;
}

void BoardView::GroupBoxesCollection::highlightGroupBoxAndDescendants(
        const int groupBoxIdToHighlight, const bool unhlighlightOtherItems) {
    // highlight `groupBoxId` & all its descendants
    int singleHighlightedCardId = -1;

    const auto [groupBoxes, cards] = boardView->groupBoxTree.getAllDescendants(groupBoxIdToHighlight);
    if (unhlighlightOtherItems) {
        boardView->nodeRectsCollection.setHighlightedCardIds(cards);
        setHighlightedGroupBoxes(groupBoxes + QSet<int> {groupBoxIdToHighlight});

        //
        singleHighlightedCardId = (cards.count() == 1) ? *cards.begin() : -1;
    }
    else {
        const QSet<int> highlightedCards
                = boardView->nodeRectsCollection.addToHighlightedCards(cards);
        addToHighlightedGroupBoxes(groupBoxes + QSet<int> {groupBoxIdToHighlight});

        //
        singleHighlightedCardId
                = (highlightedCards.count() == 1) ? *highlightedCards.begin() : -1;
    }

    // call AppData
    Services::instance()->getAppData()
            ->setSingleHighlightedCardId(EventSource(boardView), singleHighlightedCardId);
}

//======

void BoardView::ComovingStateData::activate() {
    if (isActive)
        qWarning().noquote() << "co-moving state is already active";
    isActive = true;
}

void BoardView::ComovingStateData::deactivate() {
    isActive = false;
    clearFollowers();
}

//======

void BoardView::RelationshipBundlesCollection::update() {
    relBundlesByGroupBoxId.clear();

    //
    QVector<int> groupBoxIdsFromDFS;
    const QHash<int, QSet<int>> groupBoxIdToDescendantCards
            = boardView->groupBoxTree.getDescendantCardsOfEveryGroupBox(&groupBoxIdsFromDFS);

    // consider every group-box in a topological order (predecessor first)
    QHash<int, QSet<RelationshipId>> cardIdToBundledRels;

    for (const int groupBoxId: qAsConst(groupBoxIdsFromDFS)) {
        const QSet<int> cardsWithin = groupBoxIdToDescendantCards.value(groupBoxId);

        // For every card in `cardsWithin`, find the set of possible bundles as if the group
        // contains only that card, not considering the already-bundled relationships.
        // Then take the intersection of the obtained bundle sets.
        QSet<RelationshipsBundle> bundlesOfGroup;

        for (auto it = cardsWithin.constBegin(); it != cardsWithin.constEnd(); ++it) {
            const int cardId = *it;
            // get all possible bundles as if the group contains only this card, not considering
            // the already-bundled relationships
            QSet<RelationshipsBundle> possibleBundles1;

            const auto allRels = boardView->relationshipsCollection.getAllRelationshipIds();
            for (const RelationshipId &rel: allRels) {
                if (cardIdToBundledRels.value(cardId).contains(rel)) // relationship already bundled
                    continue;

                int theOtherCard;
                const bool connectsThisCard = rel.connectsCard(cardId, &theOtherCard);
                if (!connectsThisCard)
                    continue;

                if (cardsWithin.contains(theOtherCard)) // `theOtherCard` is not external card
                    continue;

                RelationshipsBundle bundle;
                {
                    bundle.groupBoxId = groupBoxId;
                    bundle.externalCardId = theOtherCard;
                    bundle.relationshipType = rel.type;
                    bundle.direction = (cardId == rel.startCardId)
                            ? RelationshipsBundle::Direction::OutFromGroup
                            : RelationshipsBundle::Direction::IntoGroup;
                }
                possibleBundles1 << bundle;
            }

            //
            if (it == cardsWithin.constBegin()) {
                if (possibleBundles1.isEmpty())
                    break;
                bundlesOfGroup = possibleBundles1;
            }
            else {
                bundlesOfGroup &= possibleBundles1;
                if (bundlesOfGroup.isEmpty())
                    break;
            }
        }

        // add to `cardIdToBundledRels`
        for (const int cardId: cardsWithin) {
            QSet<RelationshipId> bundledRels;
            for (const RelationshipsBundle &bundle: qAsConst(bundlesOfGroup)) {
                if (bundle.direction == RelationshipsBundle::Direction::IntoGroup) {
                    bundledRels
                            << RelationshipId(bundle.externalCardId, cardId, bundle.relationshipType);
                }
                else {
                    bundledRels
                            << RelationshipId(cardId, bundle.externalCardId, bundle.relationshipType);
                }
            }
            cardIdToBundledRels[cardId] += bundledRels;
        }

        //
        relBundlesByGroupBoxId.insert(groupBoxId, bundlesOfGroup);
    }

    //
    bundledRels.clear();
    for (auto it = cardIdToBundledRels.constBegin(); it != cardIdToBundledRels.constEnd(); ++it)
        bundledRels += it.value();

    // compute `relBundlesByExternalCardId` & `groupBoxAndCardToBundles`
    QSet<RelationshipsBundle> allBundles;
    for (auto it = relBundlesByGroupBoxId.constBegin();
            it != relBundlesByGroupBoxId.constEnd(); ++it) {
        allBundles += it.value();
    }

    relBundlesByExternalCardId.clear();
    relBundlesByGroupBoxAndCard.clear();
    for (const RelationshipsBundle &bundle: qAsConst(allBundles)) {
        relBundlesByExternalCardId[bundle.externalCardId] << bundle;
        relBundlesByGroupBoxAndCard[{bundle.groupBoxId, bundle.externalCardId}] << bundle;
    }

    //
    redrawBundleArrows();
}

void BoardView::RelationshipBundlesCollection::updateBundlesConnectingGroupBox(
        const int groupBoxId) {
    const QSet<RelationshipsBundle> bundles = relBundlesByGroupBoxId.value(groupBoxId);

    QSet<int> externalCardIds;
    for (const auto &bundle: bundles)
        externalCardIds << bundle.externalCardId;

    for (const int cardId: qAsConst(externalCardIds)) {
        const QSet<RelationshipsBundle> parallelBundles
                = relBundlesByGroupBoxAndCard.value({groupBoxId, cardId});

        int index = 0;
        for (const auto &bundle: parallelBundles) {
            updateEdgeArrow(bundle, index, parallelBundles.count());
            ++index;
        }
    }
}

void BoardView::RelationshipBundlesCollection::updateBundlesConnectingNodeRect(const int cardId) {
    const QSet<RelationshipsBundle> bundles = relBundlesByExternalCardId.value(cardId);

    QSet<int> groupBoxIds;
    for (const auto &bundle: bundles)
        groupBoxIds << bundle.groupBoxId;

    for (const int groupBoxId: qAsConst(groupBoxIds)) {
        const QSet<RelationshipsBundle> parallelBundles
                = relBundlesByGroupBoxAndCard.value({groupBoxId, cardId});

        int index = 0;
        for (const auto &bundle: parallelBundles) {
            updateEdgeArrow(bundle, index, parallelBundles.count());
            ++index;
        }
    }
}

QSet<RelationshipsBundle> BoardView::RelationshipBundlesCollection::getBundlesOfGroupBox(
        const int groupBoxId) const {
    return relBundlesByGroupBoxId.value(groupBoxId);
}

QSet<RelationshipId> BoardView::RelationshipBundlesCollection::getBundledRelationships() const {
    return bundledRels;
}

QRectF BoardView::RelationshipBundlesCollection::getBoundingRectOfAllArrows() const {
    QRectF result;
    for (auto it = relBundleToEdgeArrow.constBegin();
            it != relBundleToEdgeArrow.constEnd(); ++it) {
        if (result.isNull())
            result = it.value()->boundingRect();
        else
            result = result.united(it.value()->boundingRect());
    }
    return result;
}

void BoardView::RelationshipBundlesCollection::setLineColorAndLabelColorOfAllEdgeArrows(
        const QColor &lineColor, const QColor &labelColor) {
    for (auto it = relBundleToEdgeArrow.constBegin();
            it != relBundleToEdgeArrow.constEnd(); ++it) {
        it.value()->setLineColor(lineColor);
        it.value()->setLabelColor(labelColor);
    }
}

void BoardView::RelationshipBundlesCollection::redrawBundleArrows() {
    // remove current edge arrows
    for (auto it = relBundleToEdgeArrow.constBegin(); it != relBundleToEdgeArrow.constEnd(); ++it) {
        EdgeArrow *edgeArrow = it.value();
        boardView->graphicsScene->removeItem(edgeArrow);
        delete edgeArrow;
    }
    relBundleToEdgeArrow.clear();

    //
    constexpr double lineWidth = 4;
    const QColor lineColor = boardView->getEdgeArrowLineColor();
    const QColor labelColor = boardView->getEdgeArrowLabelColor();

    for (auto it = relBundlesByGroupBoxAndCard.constBegin();
            it != relBundlesByGroupBoxAndCard.constEnd(); ++it) {
        const QSet<RelationshipsBundle> &parallelBundles = it.value();
        int index = 0;
        for (const auto &bundle: parallelBundles) {
            // create EdgeArrow
            auto *edgeArrow = new EdgeArrow(boardView->canvas);
            relBundleToEdgeArrow.insert(bundle, edgeArrow);

            edgeArrow->setZValue(zValueForEdgeArrows);
            edgeArrow->setLineWidth(lineWidth);
            edgeArrow->setLineColor(lineColor);
            edgeArrow->setLabelColor(labelColor);

            updateEdgeArrow(bundle, index, parallelBundles.count());

            //
            ++index;
        }
    }
}

void BoardView::RelationshipBundlesCollection::updateEdgeArrow(
        const RelationshipsBundle &bundle, const int parallelIndex, const int parallelCount) {
    Q_ASSERT(parallelCount >= 1);
    Q_ASSERT(parallelIndex < parallelCount);

    const QLineF line = computeEdgeArrowLine(bundle, parallelIndex, parallelCount);

    auto *edgeArrow = relBundleToEdgeArrow.value(bundle);
    edgeArrow->setStartEndPoint(line.p1(), line.p2());
    edgeArrow->setLabel(bundle.relationshipType);
}

QLineF BoardView::RelationshipBundlesCollection::computeEdgeArrowLine(
        const RelationshipsBundle &bundle, const int parallelIndex, const int parallelCount) {
    QRectF startRect;
    QRectF endRect;

    GroupBox *groupBox = boardView->groupBoxesCollection.get(bundle.groupBoxId);
    Q_ASSERT(groupBox != nullptr);
    const std::optional<QRectF> nodeRectRectOpt
            = boardView->nodeRectsCollection.getNodeRectRect(bundle.externalCardId);
    Q_ASSERT(nodeRectRectOpt.has_value());

    if (bundle.direction == RelationshipsBundle::Direction::IntoGroup) {
        startRect = nodeRectRectOpt.value();
        endRect = groupBox->getRect();
    }
    else {
        startRect = groupBox->getRect();
        endRect = nodeRectRectOpt.value();
    }
    return computeArrowLineConnectingRects(startRect, endRect, parallelIndex, parallelCount);
}

//======

BoardView::ContextMenu::ContextMenu(BoardView *boardView_)
        : boardView(boardView_) {
    menu = new QMenu(boardView);
    {
        auto *action = menu->addAction("Open Existing Card...");
        actionToIcon.insert(action, Icon::OpenInNew);
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToOpenExistingCard(requestScenePos);
        });
    }
    {
        auto *action = menu->addAction("Create New Card");
        actionToIcon.insert(action, Icon::AddBox);
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToCreateNewCard(requestScenePos);
        });
    }
    {
        auto *action = menu->addAction("Duplicate Card...");
        actionToIcon.insert(action, Icon::ContentCopy);
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToDuplicateCard(requestScenePos);
        });
    }
    menu->addSeparator();
    {
        auto *action = menu->addAction("Create Group Box");
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToCreateNewGroup(requestScenePos);
        });
    }
    {
        auto *action = menu->addAction("Create New Data Query");
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToCreateNewCustomDataQuery(requestScenePos);
        });
    }
    menu->addSeparator();
    {
        auto *action = menu->addAction("Open Settings...");
        connect(action, &QAction::triggered, boardView, [this]() {
            boardView->onUserToCreateSettingBox(requestScenePos);
        });
    }
}

void BoardView::ContextMenu::setActionIcons() {
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = actionToIcon.constBegin(); it != actionToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
}

//======

BoardView::SettingBoxesCollection::SettingBoxesCollection(BoardView *boardView)
        : boardView(boardView) {
}

void BoardView::SettingBoxesCollection::createSettingBox(
        const SettingTargetTypeAndCategory targetTypeAndCategory,
        const QRectF &rect, const QColor &displayColor,
        std::function<void (SettingBox *)> callback) {
    Q_ASSERT(callback);
    Q_ASSERT(!settingBoxes.contains(targetTypeAndCategory));

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int targetId {-1};
        std::shared_ptr<AbstractWorkspaceOrBoardSetting> settingData;
        SettingBox *settingBox {nullptr};
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, targetTypeAndCategory]() {
        switch (targetTypeAndCategory.first) {
        case SettingTargetType::Workspace:
            // set routine->targetId as the workspace ID containing current boardId
            boardView->getWorkspaceId([routine](const int workspaceId) {
                ContinuationContext context(routine);
                if (workspaceId == -1) {
                    context.setErrorFlag();
                    return;
                }
                routine->targetId = workspaceId;
            });
            return;

        case SettingTargetType::Board:
        {
            // set routine->targetId as the current boardId
            ContinuationContext context(routine);
            if (boardView->boardId == -1) {
                context.setErrorFlag();
                return;
            }
            routine->targetId = boardView->boardId;
            return;
        }
        }

        Q_ASSERT(false); // case not implemented
        routine->skipToFinalStep();
    }, boardView);

    routine->addStep([this, routine, targetTypeAndCategory]() {
        // get settings data
        switch (targetTypeAndCategory.first) {
        case SettingTargetType::Workspace:
            createSettingDataForWorkspace(
                    routine->targetId,
                    targetTypeAndCategory.second,
                    // callback:
                    [routine](AbstractWorkspaceOrBoardSetting *settingData) {
                        ContinuationContext context(routine);

                        if (settingData == nullptr) {
                            context.setErrorFlag();
                            return;
                        }
                        routine->settingData
                                = std::shared_ptr<AbstractWorkspaceOrBoardSetting>(settingData);
                    }
            );
            break;

        case SettingTargetType::Board:
            createSettingDataForBoard(
                    routine->targetId,
                    targetTypeAndCategory.second,
                    // callback:
                    [routine](AbstractWorkspaceOrBoardSetting *settingData) {
                        ContinuationContext context(routine);

                        if (settingData == nullptr) {
                            context.setErrorFlag();
                            return;
                        }
                        routine->settingData
                                = std::shared_ptr<AbstractWorkspaceOrBoardSetting>(settingData);
                    }
            );
            break;
        }
    }, boardView);

    routine->addStep([this, routine, targetTypeAndCategory, rect, displayColor]() {
        // create SettingBox
        Q_ASSERT(routine->settingData != nullptr && routine->targetId != -1);
        ContinuationContext context(routine);

        const QString title
                = QString("%1 Setting: %2")
                  .arg(getDisplayNameOfTargetType(targetTypeAndCategory.first),
                       getDisplayNameOfCategory(targetTypeAndCategory.second));
        const QString description = getDescriptionForTargetTypeAndCategory(targetTypeAndCategory);
        const QString schema = routine->settingData->schema();
        const QString settingJson = routine->settingData->toJsonStr(QJsonDocument::Indented);

        SettingBox *settingBox = new SettingBox(boardView->canvas);
        routine->settingBox = settingBox;

        settingBox->setZValue(zValueForNodeRects);
        settingBox->initialize();

        settingBox->setTitle(title);
        settingBox->setDescription(description);
        settingBox->setSchema(schema);
        settingBox->setSettingJson(settingJson);
        settingBox->setRect(rect);
        settingBox->setColor(displayColor);

        // add to `settingBoxes`
        SettingsBoxAndData boxAndData {settingBox, routine->settingData, routine->targetId};
        settingBoxes.insert(targetTypeAndCategory, boxAndData);
    }, boardView);

    routine->addStep([this, routine, targetTypeAndCategory]() {
        // set up connections
        Q_ASSERT(routine->settingData != nullptr && routine->targetId != -1);
        ContinuationContext context(routine);

        QPointer<SettingBox> settingBoxPtr(routine->settingBox);

        QObject::connect(
                routine->settingBox, &SettingBox::settingEdited,
                boardView, [this, settingBoxPtr, targetTypeAndCategory]() {
            if (!settingBoxPtr)
                return;

            editedSettings << targetTypeAndCategory;
            boardView->handleSettingsEditedDebouncer->tryAct();
            emit boardView->hasWorkspaceSettingsPendingUpdateChanged(
                    boardView->handleSettingsEditedDebouncer->hasDelayed());
        });

        QObject::connect(
                routine->settingBox, &SettingBox::finishedMovingOrResizing,
                boardView, [this, settingBoxPtr, targetTypeAndCategory]() {
            if (!settingBoxPtr)
                return;

            //
            boardView->adjustSceneRect();

            // call AppData
            SettingBoxDataUpdate update;
            update.rect = settingBoxPtr->getRect();

            Services::instance()->getAppData()->updateSettingBoxProperties(
                    EventSource(boardView), boardView->boardId,
                    targetTypeAndCategory.first, targetTypeAndCategory.second, update);
        });

        QObject::connect(
                routine->settingBox, &SettingBox::closeByUser,
                boardView, [this, settingBoxPtr, targetTypeAndCategory]() {
            if (!settingBoxPtr)
                return;
            boardView->onUserToCloseSettingBox(targetTypeAndCategory);
        });
    }, boardView);

    routine->addStep([routine, callback]() {
        // final step
        ContinuationContext context(routine);
        callback(routine->settingBox);
    }, boardView);

    routine->start();
}

void BoardView::SettingBoxesCollection::closeSettingBox(
        const SettingTargetTypeAndCategory targetTypeAndCategory) {
    if (!settingBoxes.contains(targetTypeAndCategory))
        return;
    const SettingsBoxAndData boxAndData = settingBoxes.take(targetTypeAndCategory);

    //
    boardView->graphicsScene->removeItem(boxAndData.box);
    boxAndData.box->deleteLater();

    //
    boardView->graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // This is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug
}

void BoardView::SettingBoxesCollection::updateSettingBoxIfExists(
        const SettingTargetTypeAndCategory targetTypeAndCategory, const int targetId) {
    if (!settingBoxes.contains(targetTypeAndCategory))
        return;
    if (settingBoxes.value(targetTypeAndCategory).targetId != targetId)
        return;

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        std::shared_ptr<AbstractSetting> settingData;
    };

    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, targetTypeAndCategory, targetId]() {
        // get settings data
        switch (targetTypeAndCategory.first) {
        case SettingTargetType::Workspace:
            createSettingDataForWorkspace(
                    targetId,
                    targetTypeAndCategory.second,
                    // callback:
                    [routine](AbstractWorkspaceOrBoardSetting *settingData) {
                        ContinuationContext context(routine);

                        if (settingData == nullptr) {
                            context.setErrorFlag();
                            return;
                        }
                        routine->settingData
                                = std::shared_ptr<AbstractWorkspaceOrBoardSetting>(settingData);
                    }
            );
            break;

        case SettingTargetType::Board:
            createSettingDataForBoard(
                    targetId,
                    targetTypeAndCategory.second,
                    // callback:
                    [routine](AbstractWorkspaceOrBoardSetting *settingData) {
                        ContinuationContext context(routine);

                        if (settingData == nullptr) {
                            context.setErrorFlag();
                            return;
                        }
                        routine->settingData
                                = std::shared_ptr<AbstractWorkspaceOrBoardSetting>(settingData);
                    }
            );
            break;
        }

    }, boardView);

    routine->addStep([this, routine, targetTypeAndCategory]() {
        ContinuationContext context(routine);

        //
        settingBoxes[targetTypeAndCategory].data = routine->settingData;

        // reload SettingBox
        settingBoxes[targetTypeAndCategory].box->setSettingJson(
                routine->settingData->toJsonStr(QJsonDocument::Indented));
        settingBoxes[targetTypeAndCategory].box->setErrorMsg("");
    }, boardView);

    routine->addStep([routine]() {
        // final step
        ContinuationContext context(routine);
    }, boardView);

    routine->start();
}

void BoardView::SettingBoxesCollection::updateAllSettingBoxesColors() {
    const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
    const bool autoAdjustCardColorsForDarkTheme
            = Services::instance()->getAppDataReadonly()->getAutoAdjustCardColorsForDarkTheme();
    const QColor displayColor = computeSettingBoxDisplayColor(
            defaultNewSettingBoxColor, autoAdjustCardColorsForDarkTheme && isDarkTheme);

    for (auto it = settingBoxes.constBegin(); it != settingBoxes.constEnd(); ++it)
        it.value().box->setColor(displayColor);
}

void BoardView::SettingBoxesCollection::setAllSettingBoxesTextEditorIgnoreWheelEvent(const bool b) {
    for (auto it = settingBoxes.begin(); it != settingBoxes.end(); ++it)
        it.value().box->setTextEditorIgnoreWheelEvent(b);
}

void BoardView::SettingBoxesCollection::handleEditedSettings() {
    for (const auto &targetTypeAndCategory: qAsConst(editedSettings)) {
        if (!settingBoxes.contains(targetTypeAndCategory))
            continue;

        const SettingsBoxAndData boxAndData = settingBoxes.value(targetTypeAndCategory);
        const QString settingText = boxAndData.box->getSettingText();

        // validate
        QString errorMsg;
        const bool ok = boxAndData.data->validate(settingText, &errorMsg);
        if (!ok) {
            if (errorMsg.isEmpty())
                errorMsg = "Invalid setting.";
            boxAndData.box->setErrorMsg(errorMsg);
        }
        else {
            // validation OK, apply the setting
            boxAndData.box->setErrorMsg("");
            const bool ok1 = boxAndData.data->setFromJsonStr(settingText);
            Q_ASSERT(ok1);

            switch (targetTypeAndCategory.first) {
            case SettingTargetType::Workspace:
                applyWorkspaceSetting(boxAndData.targetId, boxAndData.data);
                break;

            case SettingTargetType::Board:
                applyBoardSetting(boxAndData.targetId, boxAndData.data);
                break;
            }
        }
    }

    //
    editedSettings.clear();
    emit boardView->hasWorkspaceSettingsPendingUpdateChanged(false);
}

QSet<SettingTargetTypeAndCategory> BoardView::SettingBoxesCollection::getAllSettingBoxes() const {
    return keySet(settingBoxes);
}

bool BoardView::SettingBoxesCollection::containsSetting(
        const SettingTargetTypeAndCategory targetTypeAndCategory) const {
    return settingBoxes.contains(targetTypeAndCategory);
}

QRectF BoardView::SettingBoxesCollection::getBoundingRectOfAllSettingBoxes() const {
    QRectF result;
    for (auto it = settingBoxes.constBegin(); it != settingBoxes.constEnd(); ++it) {
        if (result.isNull())
            result = it.value().box->boundingRect();
        else
            result = result.united(it.value().box->boundingRect());
    }
    return result;
}

void BoardView::SettingBoxesCollection::createSettingDataForWorkspace(
        const int workspaceId, const SettingCategory category,
        std::function<void (AbstractSetting *setting)> callback) {
    // Get the setting data (value) for `category` for `workspaceId`
    Services::instance()->getAppDataReadonly()->getWorkspaces(
            // callback
            [workspaceId, category, callback](bool ok, const QHash<int, Workspace> &workspaces) {
                if (!ok) {
                    callback(nullptr);
                    return;
                }
                if (!workspaces.contains(workspaceId)) {
                    callback(nullptr);
                    return;
                }
                const Workspace workspace = workspaces.value(workspaceId);

                //
                switch (category) {
                case SettingCategory::CardLabelToColorMapping:
                {
                    auto *cardLabelToColorMapping = new CardLabelToColorMapping;
                    *cardLabelToColorMapping = workspace.cardLabelToColorMapping;
                    callback(cardLabelToColorMapping);
                    return;
                }
                case SettingCategory::CardPropertiesToShow:
                {
                    auto *cardpropertiesToShow = new CardPropertiesToShow;
                    *cardpropertiesToShow = workspace.cardPropertiesToShow;
                    callback(cardpropertiesToShow);
                    return;
                }
                case SettingCategory::WorkspaceSchema:
                {
                    callback(nullptr); // [temp]
                    return;
                }
                }

                Q_ASSERT(false); // case not implemented
                callback(nullptr);
            },
            boardView
    );
}

void BoardView::SettingBoxesCollection::createSettingDataForBoard(
        const int boardId, const SettingCategory category,
        std::function<void (AbstractSetting *setting)> callback) {
    switch (category) {
    case SettingCategory::CardPropertiesToShow:
        Services::instance()->getAppData()->getBoardData(
                boardId,
                // callback
                [callback](bool ok, std::optional<Board> boardData) {
                    if (!ok) {
                        callback(nullptr);
                        return;
                    }
                    if (!boardData.has_value()) {
                        callback(nullptr);
                        return;
                    }

                    auto *cardPropertiesToShow = new CardPropertiesToShow;
                    *cardPropertiesToShow = boardData.value().cardPropertiesToShow;
                    callback(cardPropertiesToShow);
                },
                boardView
        );
        return;

    case SettingCategory::CardLabelToColorMapping: [[fallthrough]];
    case SettingCategory::WorkspaceSchema:
        Q_ASSERT(false); // category is not for boards
        callback(nullptr);
        return;
    }

    Q_ASSERT(false); // case not implemented
    callback(nullptr);
}

void BoardView::SettingBoxesCollection::applyWorkspaceSetting(
        const int workspaceId, std::shared_ptr<AbstractSetting> settingData) {
    switch (settingData->category) {
    case SettingCategory::CardLabelToColorMapping:
    {
        auto *cardLabelToColorMapping = dynamic_cast<CardLabelToColorMapping *>(settingData.get());
        Q_ASSERT(cardLabelToColorMapping != nullptr);
        emit boardView->workspaceCardLabelToColorMappingUpdatedViaSettingBox(
                workspaceId, *cardLabelToColorMapping);
        break;
    }

    case SettingCategory::CardPropertiesToShow:
    {
        auto *cardPropertiesToShow = dynamic_cast<CardPropertiesToShow *>(settingData.get());
        Q_ASSERT(cardPropertiesToShow != nullptr);
        emit boardView->workspaceCardPropertiesToShowUpdatedViaSettingBox(
                workspaceId, *cardPropertiesToShow);
        break;
    }
    case SettingCategory::WorkspaceSchema:
    {
        break;
    }
    }
}

void BoardView::SettingBoxesCollection::applyBoardSetting(
        const int boardId, std::shared_ptr<AbstractSetting> settingData) {
    Q_ASSERT(boardView->boardId != -1);

    if (settingData->category == SettingCategory::CardPropertiesToShow) {
        auto *cardPropertiesToShow = dynamic_cast<CardPropertiesToShow *>(settingData.get());
        Q_ASSERT(cardPropertiesToShow != nullptr);

        if (boardId != boardView->boardId) {
            qWarning().noquote()
                    << "SettingBox edits setting of a board other than the current one";
            return;
        }

        boardView->cardPropertiesToShowSettings.onBoard = *cardPropertiesToShow;
        boardView->updatePropertiesDisplayOfAllCards();

        //
        BoardNodePropertiesUpdate update;
        update.cardPropertiesToShow = *cardPropertiesToShow;

        Services::instance()->getAppData()->updateBoardNodeProperties(
                EventSource(boardView), boardId, update);
    }
}
