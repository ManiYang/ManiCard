#include <utility>
#include <QDebug>
#include <QGraphicsView>
#include <QInputDialog>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "app_data.h"
#include "board_view.h"
#include "persisted_data_access.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/binary_search.h"
#include "utilities/geometry_util.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"
#include "utilities/numbers_util.h"
#include "utilities/periodic_checker.h"
#include "utilities/strings_util.h"
#include "widgets/board_view_toolbar.h"
#include "widgets/components/edge_arrow.h"
#include "widgets/components/graphics_scene.h"
#include "widgets/components/node_rect.h"
#include "widgets/dialogs/dialog_board_card_colors.h"
#include "widgets/dialogs/dialog_create_relationship.h"
#include "widgets/dialogs/dialog_set_labels.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

BoardView::BoardView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpContextMenu();
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

    // close all cards
    bool highlightedCardIdChanged = false; // to -1

    if (boardId != -1) {
        if (!canClose()) {
            callback(false, false);
            return;
        }

        closeAllCards(&highlightedCardIdChanged);
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
    };
    auto *routine = new AsyncRoutineWithVars;
    routine->setName("BoardView::loadBoard");

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
        // 3. open cards, creating NodeRect's
        ContinuationContext context(routine);

        for (auto it = routine->cardsData.constBegin();
                it != routine->cardsData.constEnd(); ++it) {
            const int &cardId = it.key();
            const Card &cardData = it.value();

            const NodeRectData nodeRectData = routine->board.cardIdToNodeRectData.value(cardId);

            const QColor displayColor = computeNodeRectDisplayColor(
                    nodeRectData.ownColor, cardData.getLabels(),
                    routine->board.cardLabelsAndAssociatedColors,
                    routine->board.defaultNodeRectColor);

            NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                    cardId, cardData, nodeRectData.rect,
                    displayColor, nodeRectData.ownColor,
                    routine->userLabelsList);
//            nodeRect->set({{NodeRect::InputVar::IsEditable, true}});
            nodeRect->setEditable(true);
        }

        //
        zoomScale = routine->board.zoomRatio;
        canvas->setScale(zoomScale * graphicsGeometryScaleFactor); // (1)
        adjustSceneRect(); // (2)
        setViewTopLeftPos(routine->board.topLeftPos); // (3)
    }, this);

    routine->addStep([this, routine]() {
        // 4. get relationships
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
        // 5. create EdgeArrow's
        ContinuationContext context(routine);

        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = defaultEdgeArrowLineColor;
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
        }

        const auto relIds = keySet(routine->relationshipsData);
        for (const auto &relId: relIds)
            edgeArrowsCollection.createEdgeArrow(relId, edgeArrowData);
    }, this);

    routine->addStep([this, routine, callback, boardIdToLoad, highlightedCardIdChanged]() {
        // final step
        ContinuationContext context(routine);

        if (!routine->errorFlag) {
            boardId = boardIdToLoad;
            boardName = routine->board.name;
            cardLabelsAndAssociatedColors = routine->board.cardLabelsAndAssociatedColors;
            defaultNodeRectColor = routine->board.defaultNodeRectColor;
        }

        callback(!routine->errorFlag, highlightedCardIdChanged);
    }, this);

    routine->start();
}

void BoardView::prepareToClose() {
    // do nothing
}

void BoardView::showButtonRightSidebar() {
    toolBar->showButtonOpenRightSidebar();
}

void BoardView::applyZoomAction(const ZoomAction zoomAction) {
    const auto anchorScenePos = getViewCenterInScene();
    doApplyZoomAction(zoomAction, anchorScenePos);
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

bool BoardView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == graphicsView) {
        if (event->type() == QEvent::Resize)
            adjustSceneRect();
    }
    return false;
}

void BoardView::setUpWidgets() {
    const QColor sceneBackgroundColor(230, 230, 230);

    //
    this->setFrameShape(QFrame::NoFrame);

    // set up layout
    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    this->setLayout(layout);
    {
        toolBar = new BoardViewToolBar(this);
        layout->addWidget(toolBar);

        graphicsView = new QGraphicsView;
        layout->addWidget(graphicsView);
    }

    // set up `graphicsScene`, & `canvas`
    graphicsScene = new GraphicsScene(this);
    graphicsScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    graphicsScene->setBackgroundBrush(sceneBackgroundColor);

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

void BoardView::setUpContextMenu() {
    contextMenu = new QMenu(this);
    {
        QAction *action = contextMenu->addAction(
                QIcon(":/icons/open_in_new_black_24"), "Open Existing Card...");
        connect(action, &QAction::triggered, this, [this]() {
            onUserToOpenExistingCard(contextMenuData.requestScenePos);
        });
    }
    {
        QAction *action = contextMenu->addAction(
                QIcon(":/icons/add_box_black_24"), "Create New Card...");
        connect(action, &QAction::triggered, this, [this]() {
            onUserToCreateNewCard(contextMenuData.requestScenePos);
        });
    }
    {
        QAction *action = contextMenu->addAction(
                QIcon(":/icons/content_copy_24"), "Duplicate Card...");
        connect(action, &QAction::triggered, this, [this]() {
            onUserToDuplicateCard(contextMenuData.requestScenePos);
        });
    }
}

void BoardView::setUpConnections() {
    // Connections from newly created NodeRect's are established in
    // NodeRectsCollection::createNodeRect().

    //
    connect(graphicsScene, &GraphicsScene::contextMenuRequestedOnScene,
            this, [this](const QPointF &scenePos) {
        contextMenuData.requestScenePos = scenePos;
        contextMenu->popup(getScreenPosFromScenePos(scenePos));
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
    });

    connect(graphicsScene, &GraphicsScene::viewScrollingFinished, this, [this]() {
        nodeRectsCollection.setAllNodeRectsTextEditorIgnoreWheelEvent(false);
    });

    //
    connect(toolBar, &BoardViewToolBar::openRightSidebar, this, [this]() {
        emit openRightSideBar();
    });

    connect(toolBar, &BoardViewToolBar::openCardColorsDialog, this, [this]() {
        onUserToSetCardColors();
    });

    //
    connect(Services::instance()->getAppData(), &AppData::fontSizeScaleFactorChanged,
            this, [this](const QWidget *window, const double factor) {
        if (this->window() != window)
            return;
        if (std::fabs(factor - graphicsGeometryScaleFactor) < 1e-3)
            return;

        graphicsGeometryScaleFactor = factor;
        updateCanvasScale(zoomScale * graphicsGeometryScaleFactor, getViewCenterInScene());
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

        routine->nodeRectData.rect
                = QRectF(canvas->mapFromScene(scenePos), defaultNewNodeRectSize);
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, routine->cardData.getLabels(),
                cardLabelsAndAssociatedColors, defaultNodeRectColor);

        auto *nodeRect = nodeRectsCollection.createNodeRect(
                cardId, routine->cardData, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList);
//        nodeRect->set({{NodeRect::InputVar::IsEditable, true}});
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
        // create EdgeArrow's
        ContinuationContext context(routine);

        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = defaultEdgeArrowLineColor;
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
        }

        for (auto it = routine->rels.constBegin(); it != routine->rels.constEnd(); ++it) {
            const auto &relId = it.key();

            int otherCardId;
            bool b = relId.connectsCard(cardId, &otherCardId);
            Q_ASSERT(b);
            if (!nodeRectsCollection.contains(otherCardId)) // `otherCardId` not opened in this board
                continue;

            edgeArrowsCollection.createEdgeArrow(relId, edgeArrowData);
        }
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
        routine->card.title = "New Card";
        routine->card.text = "";
        const QSet<QString> cardLabels {};

        ContinuationContext context(routine);

        routine->nodeRectData.rect
                = QRectF(canvas->mapFromScene(scenePos), defaultNewNodeRectSize);
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, cardLabels, cardLabelsAndAssociatedColors,
                defaultNodeRectColor);

        NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                routine->newCardId, routine->card, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList);
//        nodeRect->set({{NodeRect::InputVar::IsEditable, true}});
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

    routine->addStep([this, routine, scenePos]() {
        // create new NodeRect
        ContinuationContext context(routine);

        routine->nodeRectData.rect
                = QRectF(canvas->mapFromScene(scenePos), defaultNewNodeRectSize);
        routine->nodeRectData.ownColor = QColor();

        const QColor displayColor = computeNodeRectDisplayColor(
                routine->nodeRectData.ownColor, routine->cardData.getLabels(),
                cardLabelsAndAssociatedColors, defaultNodeRectColor);

        NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                routine->newCardId, routine->cardData, routine->nodeRectData.rect,
                displayColor, routine->nodeRectData.ownColor,
                routine->userLabelsList);
//        nodeRect->set({{NodeRect::InputVar::IsEditable, true}});
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

        Q_ASSERT(routine->updatedLabels.has_value());
        const QStringList updatedLabels = routine->updatedLabels.value();

        auto *nodeRect = nodeRectsCollection.get(cardId);

        const QColor nodeRectColor = computeNodeRectDisplayColor(
                nodeRectsCollection.getNodeRectOwnColor(cardId),
                QSet<QString>(updatedLabels.cbegin(), updatedLabels.cend()),
                cardLabelsAndAssociatedColors, defaultNodeRectColor);

//        nodeRect->set({
//            {NodeRect::InputVar::NodeLabels, updatedLabels},
//            {NodeRect::InputVar::Color, nodeRectColor}
//        });
        nodeRect->setNodeLabels(updatedLabels);
        nodeRect->setColor(nodeRectColor);

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
        // create EdgeArrow
        ContinuationContext context(routine);

        if (!nodeRectsCollection.contains(routine->relIdToCreate.startCardId)
                || !nodeRectsCollection.contains(routine->relIdToCreate.endCardId)) {
            // (one of the start/end cards is not opened in this board)
            QMessageBox::information(
                    this, " ",
                    QString("Relationship %1 created.").arg(routine->relIdToCreate.toString()));
            return;
        }

        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = defaultEdgeArrowLineColor;
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
        }
        edgeArrowsCollection.createEdgeArrow(routine->relIdToCreate, edgeArrowData);
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

    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        std::optional<int> updatedHighlightedCardId;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine, cardId]() {
        bool highlightedCardIdChanged;
        constexpr bool removeConnectedEdgeArrows = true;
        nodeRectsCollection.closeNodeRect(
                cardId, removeConnectedEdgeArrows, &highlightedCardIdChanged);

        if (highlightedCardIdChanged)
            routine->updatedHighlightedCardId = -1;

        adjustSceneRect();

        routine->nextStep();
    }, this);

    routine->addStep([this, routine]() {
        // call AppData
        if (routine->updatedHighlightedCardId.has_value()) {
            Services::instance()->getAppData()->setHighlightedCardId(
                    EventSource(this), routine->updatedHighlightedCardId.value());
        }
        routine->nextStep();
    }, this);

    routine->addStep([this, routine, cardId]() {
        // can be merged with previous step...
        // call AppData
        Services::instance()->getAppData()
                ->removeNodeRect(EventSource(this), boardId, cardId);
        routine->nextStep();
    }, this);

    routine->start();
}

void BoardView::onUserToSetCardColors() {
    auto *dialog = new DialogBoardCardColors(
            boardName, cardLabelsAndAssociatedColors, defaultNodeRectColor, this);
    connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
        dialog->deleteLater();

        if (result != QDialog::Accepted)
            return;

        //
        defaultNodeRectColor = dialog->getDefaultColor();
        cardLabelsAndAssociatedColors = dialog->getCardLabelsAndAssociatedColors();
        nodeRectsCollection.updateAllNodeRectColors();

        // call AppData
        BoardNodePropertiesUpdate propertiesUpdate;
        {
            propertiesUpdate.defaultNodeRectColor = defaultNodeRectColor;
            propertiesUpdate.cardLabelsAndAssociatedColors = cardLabelsAndAssociatedColors;
        }
        Services::instance()->getAppData()->updateBoardNodeProperties(
                EventSource(this), boardId, propertiesUpdate);
    });
    dialog->open();
}

void BoardView::onBackgroundClicked() {
    bool highlightedCardIdChanged; // to -1
    nodeRectsCollection.unhighlightAllCards(&highlightedCardIdChanged);

    if (highlightedCardIdChanged) {
        // call AppData
        constexpr int highlightedCardId = -1;
        Services::instance()->getAppData()
                ->setHighlightedCardId(EventSource(this), highlightedCardId);
    }
}

void BoardView::closeAllCards(bool *highlightedCardIdChanged_) {
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

    const auto relIds = edgeArrowsCollection.getAllRelationshipIds();
    edgeArrowsCollection.removeEdgeArrows(relIds);
}

void BoardView::adjustSceneRect() {
    QGraphicsScene *scene = graphicsView->scene();
    if (scene == nullptr)
        return;

    const QRectF contentsRectInCanvas
            = nodeRectsCollection.getBoundingRectOfAllNodeRects(); // in canvas coordinates
    const QRectF contentsRectInScene = contentsRectInCanvas.isNull()
            ? QRectF(0, 0, 10, 10)
            : QRectF(
                  canvas->mapToScene(contentsRectInCanvas.topLeft()),
                  canvas->mapToScene(contentsRectInCanvas.bottomRight()));

    // finite margins prevent user from drag-scrolling too far away from contents
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
        const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &boardDefaultColor) {
    // 1. NodeRect's own color
    if (nodeRectOwnColor.isValid())
        return nodeRectOwnColor;

    // 2. card labels & board's `cardLabelsAndAssociatedColors`
    for (const auto &labelAndColor: cardLabelsAndAssociatedColors) {
        if (cardLabels.contains(labelAndColor.first))
            return labelAndColor.second;
    }

    // 3. board's default
    if (boardDefaultColor.isValid())
        return boardDefaultColor;

    //
    return QColor(170, 170, 170);
}

QSet<RelationshipId> BoardView::getEdgeArrowsConnectingNodeRect(const int cardId) {
    QSet<RelationshipId> result;

    const auto relIds = edgeArrowsCollection.getAllRelationshipIds();
    for (const RelationshipId &relId: relIds) {
        if (relId.connectsCard(cardId))
            result << relId;
    }

    return result;
}

//====

NodeRect *BoardView::NodeRectsCollection::createNodeRect(
        const int cardId, const Card &cardData, const QRectF &rect,
        const QColor &displayColor, const QColor &nodeRectOwnColor,
        const QStringList &userLabelsList) {
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
//    nodeRect->set({
//        {NodeRect::InputVar::NodeLabels, QStringList(nodeLabelsVec.cbegin(), nodeLabelsVec.cend())},
//        {NodeRect::InputVar::Rect, rect},
//        {NodeRect::InputVar::Color, displayColor},
//    });
    nodeRect->setNodeLabels(QStringList(nodeLabelsVec.cbegin(), nodeLabelsVec.cend()));
    nodeRect->setRect(rect);
    nodeRect->setColor(displayColor);

    // set up connections
    QPointer<NodeRect> nodeRectPtr(nodeRect);
    QObject::connect(nodeRect, &NodeRect::movedOrResized, boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        // update edge arrows
        const QSet<RelationshipId> relIds
                = boardView->getEdgeArrowsConnectingNodeRect(nodeRectPtr->getCardId());
        for (const auto &relId: relIds) {
            constexpr bool updateOtherEdgeArrows = false;
            boardView->edgeArrowsCollection.updateEdgeArrow(relId, updateOtherEdgeArrows);
        }
    });

    QObject::connect(nodeRect, &NodeRect::finishedMovingOrResizing,
                     boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        //
        boardView->adjustSceneRect();

        // call AppData
        NodeRectDataUpdate update;
        update.rect = nodeRectPtr->getRect();

        Services::instance()->getAppData()->updateNodeRectProperties(
                EventSource(boardView),
                boardView->boardId, nodeRectPtr->getCardId(), update);
    });

    QObject::connect(nodeRect, &NodeRect::clicked, boardView, [this, nodeRectPtr]() {
        if (nodeRectPtr.isNull())
            return;

        // highlight `nodeRectPtr` (if not yet) and un-highlight the other NodeRect's
        const int cardIdClicked = nodeRectPtr->getCardId();
        if (nodeRectPtr->getIsHighlighted()) {
            Q_ASSERT(highlightedCardId == cardIdClicked);
            return;
        }

        for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
//            it.value()->set({{NodeRect::InputVar::IsHighlighted, (it.key() == cardIdClicked)}});
            it.value()->setIsHighlighted(it.key() == cardIdClicked);
        }

        //
        highlightedCardId = cardIdClicked;

        // call AppData
        Services::instance()->getAppData()
                ->setHighlightedCardId(EventSource(boardView), highlightedCardId);
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
    cardIdToNodeRect.remove(cardId);

    //
    boardView->graphicsScene->removeItem(nodeRect);
    nodeRect->deleteLater();

    if (removeConnectedEdgeArrows) {
        const auto relIds = boardView->getEdgeArrowsConnectingNodeRect(cardId);
        boardView->edgeArrowsCollection.removeEdgeArrows(relIds);
    }

    //
    boardView->graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // This is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug

    //
    if (highlightedCardId == cardId) {
        highlightedCardId = -1;
        *highlightedCardIdUpdated = true;
    }
}

void BoardView::NodeRectsCollection::unhighlightAllCards(bool *highlightedCardIdChanged) {
    *highlightedCardIdChanged = false;

    if (highlightedCardId != -1) {
        Q_ASSERT(cardIdToNodeRect.contains(highlightedCardId));
//        cardIdToNodeRect.value(highlightedCardId)
//                ->set({{NodeRect::InputVar::IsHighlighted, false}});
        cardIdToNodeRect.value(highlightedCardId)->setIsHighlighted(false);

        highlightedCardId = -1;
        *highlightedCardIdChanged = true;
    }
}

void BoardView::NodeRectsCollection::updateAllNodeRectColors() {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        const int cardId = it.key();
        NodeRect *nodeRect = it.value();

        const QColor color = computeNodeRectDisplayColor(
                cardIdToNodeRectOwnColor.value(cardId, QColor()),
                nodeRect->getNodeLabels(),
                boardView->cardLabelsAndAssociatedColors, boardView->defaultNodeRectColor);
//        nodeRect->set({{NodeRect::InputVar::Color, color}});
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

QSet<int> BoardView::NodeRectsCollection::getAllCardIds() const {
    return keySet(cardIdToNodeRect);
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

EdgeArrow *BoardView::EdgeArrowsCollection::createEdgeArrow(
        const RelationshipId relId, const EdgeArrowData &edgeArrowData) {
    Q_ASSERT(!relIdToEdgeArrow.contains(relId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.startCardId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.endCardId));

    auto *edgeArrow = new EdgeArrow(relId, boardView->canvas);
    relIdToEdgeArrow.insert(relId, edgeArrow);
    cardIdPairToParallelRels[QSet<int> {relId.startCardId, relId.endCardId}] << relId;

    edgeArrow->setZValue(zValueForEdgeArrows);

    constexpr bool updateOtherEdgeArrows = true;
    updateEdgeArrow(relId, updateOtherEdgeArrows);

    edgeArrow->setLineWidth(edgeArrowData.lineWidth);
    edgeArrow->setLineColor(edgeArrowData.lineColor);

    //
    return edgeArrow;
}

void BoardView::EdgeArrowsCollection::updateEdgeArrow(
        const RelationshipId &relId, const bool updateOtherEdgeArrows) {
    Q_ASSERT(relIdToEdgeArrow.contains(relId));

    const QSet<RelationshipId> allRelIds // all relationships connecting the 2 cards
            = cardIdPairToParallelRels.value(QSet<int> {relId.startCardId, relId.endCardId});
    Q_ASSERT(allRelIds.contains(relId));
    const QVector<RelationshipId> allRelIdsSorted = sortRelationshipIds(allRelIds);

    for (int i = 0; i < allRelIdsSorted.count(); ++i) {
        const auto relId1 = allRelIdsSorted.at(i);
        const bool update = (relId1 == relId) || updateOtherEdgeArrows;
        if (update)
            updateSingleEdgeArrow(relId1, i, allRelIdsSorted.count());
    }
}

void BoardView::EdgeArrowsCollection::removeEdgeArrows(const QSet<RelationshipId> &relIds) {
    for (const auto &relId: relIds) {
        if (!relIdToEdgeArrow.contains(relId))
            continue;

        EdgeArrow *edgeArrow = relIdToEdgeArrow.take(relId);
        boardView->graphicsScene->removeItem(edgeArrow);
        delete edgeArrow;

        const QSet<int> cardIdPair {relId.startCardId, relId.endCardId};
        cardIdPairToParallelRels[cardIdPair].remove(relId);
        if (cardIdPairToParallelRels[cardIdPair].isEmpty())
            cardIdPairToParallelRels.remove(cardIdPair);
    }
}

QSet<RelationshipId> BoardView::EdgeArrowsCollection::getAllRelationshipIds() const {
    return keySet(relIdToEdgeArrow);
}

//!
//! \param relId
//! \param parallelIndex: index of \e relId in the sorted list of all parallel relationships
//! \param parallelCount: number of all parallel relationships
//!
void BoardView::EdgeArrowsCollection::updateSingleEdgeArrow(
        const RelationshipId &relId, const int parallelIndex, const int parallelCount) {
    Q_ASSERT(parallelCount >= 1);
    Q_ASSERT(parallelIndex < parallelCount);

    const QLineF line = computeEdgeArrowLine(relId, parallelIndex, parallelCount);

    auto *edgeArrow = relIdToEdgeArrow.value(relId);
    edgeArrow->setStartEndPoint(line.p1(), line.p2());
    edgeArrow->setLabel(relId.type);
}

QVector<RelationshipId> BoardView::EdgeArrowsCollection::sortRelationshipIds(
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

QLineF BoardView::EdgeArrowsCollection::computeEdgeArrowLine(
        const RelationshipId &relId, const int parallelIndex, const int parallelCount) {
    // Compute the vector for translating the center-to-center line. The result must be the
    // same for all relationships connecting the same pair of cards.
    QPointF vecTranslation;
    {
        constexpr double spacing = 22;
        const double shiftDistance = (parallelIndex - (parallelCount - 1.0) / 2.0) * spacing;

        //
        const int card1 = std::min(relId.startCardId, relId.endCardId);
        const int card2 = std::max(relId.startCardId, relId.endCardId);

        const QPointF center1
                = boardView->nodeRectsCollection.get(card1)->getRect().center();
        const QPointF center2
                = boardView->nodeRectsCollection.get(card2)->getRect().center();
        const QLineF lineNormal = QLineF(center1, center2).normalVector().unitVector();
        const QPointF vecNormal(lineNormal.dx(), lineNormal.dy());

        //
        vecTranslation = vecNormal * shiftDistance;
    }

    //
    const QRectF startNodeRect = boardView->nodeRectsCollection.get(relId.startCardId)->getRect();
    const QRectF endNodeRect = boardView->nodeRectsCollection.get(relId.endCardId)->getRect();

    const QLineF lineC2CTranslated
            = QLineF(startNodeRect.center(), endNodeRect.center())
              .translated(vecTranslation);

    //
    bool intersect;
    QPointF intersectionPoint;

    intersect = rectEdgeIntersectsWithLine(startNodeRect, lineC2CTranslated, &intersectionPoint);
    const QPointF startPoint = intersect ?  intersectionPoint : startNodeRect.center();

    intersect = rectEdgeIntersectsWithLine(endNodeRect, lineC2CTranslated, &intersectionPoint);
    const QPointF endPoint = intersect ?  intersectionPoint : endNodeRect.center();

    return {startPoint, endPoint};
}
