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
#include "utilities/geometry_util.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "utilities/strings_util.h"
#include "widgets/board_view_toolbar.h"
#include "widgets/components/edge_arrow.h"
#include "widgets/components/graphics_scene.h"
#include "widgets/components/node_rect.h"
#include "widgets/dialogs/dialog_board_card_colors.h"
#include "widgets/dialogs/dialog_create_relationship.h"
#include "widgets/dialogs/dialog_set_labels.h"

using StringOpt = std::optional<QString>;
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

            const QColor nodeRectColor = computeNodeRectColor(
                    nodeRectData.color, cardData.getLabels(),
                    routine->board.cardLabelsAndAssociatedColors,
                    routine->board.defaultNodeRectColor);

            NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                    cardId, cardData, nodeRectData.rect, nodeRectColor,
                    routine->userLabelsList);
            nodeRect->setEditable(true);
        }

        //
        adjustSceneRect();
        setViewTopLeftPos(routine->board.topLeftPos);
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
//    const auto nodeRects = nodeRectsCollection.getAllNodeRects();
//    for (NodeRect *nodeRect: nodeRects)
//        nodeRect->prepareToClose();
}

void BoardView::showButtonRightSidebar() {
    toolBar->showButtonOpenRightSidebar();
}

int BoardView::getBoardId() const {
    return boardId;
}

QPointF BoardView::getViewTopLeftPos() const {
    return graphicsView->mapToScene(0, 0);
}

bool BoardView::canClose() const {
//    const auto nodeRects = nodeRectsCollection.getAllNodeRects();
//    for (NodeRect *nodeRect: nodeRects) {
//        if (!nodeRect->canClose())
//            return false;
//    }
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

    // set up `graphicsView`
    graphicsScene = new GraphicsScene(this);
    graphicsScene->setBackgroundBrush(sceneBackgroundColor);
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
                QIcon(":/icons/add_box_black_24"), "Create New Card");
        connect(action, &QAction::triggered, this, [this]() {
            onUserToCreateNewCard(contextMenuData.requestScenePos);
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

    //
    connect(toolBar, &BoardViewToolBar::openRightSidebar, this, [this]() {
        emit openRightSideBar();
    });

    connect(toolBar, &BoardViewToolBar::openCardColorsDialog, this, [this]() {
        onUserToSetCardColors();
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

        routine->nodeRectData.rect = QRectF(scenePos, defaultNewNodeRectSize);
        routine->nodeRectData.color = QColor();

        const QColor nodeRectColor = computeNodeRectColor(
                routine->nodeRectData.color, routine->cardData.getLabels(),
                cardLabelsAndAssociatedColors, defaultNodeRectColor);

        auto *nodeRect = nodeRectsCollection.createNodeRect(
                cardId, routine->cardData, routine->nodeRectData.rect, nodeRectColor,
                routine->userLabelsList);
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

        routine->nodeRectData.rect = QRectF(scenePos, defaultNewNodeRectSize);
        routine->nodeRectData.color = QColor();

        const QColor nodeRectColor = computeNodeRectColor(
                routine->nodeRectData.color, cardLabels, cardLabelsAndAssociatedColors,
                defaultNodeRectColor);

        NodeRect *nodeRect = nodeRectsCollection.createNodeRect(
                routine->newCardId, routine->card, routine->nodeRectData.rect, nodeRectColor,
                routine->userLabelsList);
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
        nodeRect->setNodeLabels(updatedLabels);

        const QColor nodeRectOwnColor {}; // [temp]........
        const QColor nodeRectColor = computeNodeRectColor(
                nodeRectOwnColor, QSet<QString>(updatedLabels.cbegin(), updatedLabels.cend()),
                cardLabelsAndAssociatedColors, defaultNodeRectColor);
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

    QRectF contentsRect = scene->itemsBoundingRect();
    if (contentsRect.isEmpty())
        contentsRect = QRectF(0, 0, 10, 10); // x,y,w,h

    // finite margins prevent user from drag-scrolling too far away from contents
    constexpr double fraction = 0.8;
    const double marginX = graphicsView->width() * fraction;
    const double marginY = graphicsView->height() * fraction;

    //
    const auto sceneRect
            = contentsRect.marginsAdded(QMarginsF(marginX, marginY,marginX, marginY));
    graphicsView->setSceneRect(sceneRect);
}

QPoint BoardView::getScreenPosFromScenePos(const QPointF &scenePos) {

    QPoint posInViewport = graphicsView->mapFromScene(scenePos);
    return graphicsView->viewport()->mapToGlobal(posInViewport);
}

void BoardView::setViewTopLeftPos(const QPointF &scenePos) {
    const double centerX = scenePos.x() + graphicsView->viewport()->width() * 0.5;
    const double centerY = scenePos.y() + graphicsView->viewport()->height() * 0.5;
    graphicsView->centerOn(centerX, centerY);
}

QColor BoardView::computeNodeRectColor(
        const QColor &nodeRectColor, const QSet<QString> &cardLabels,
        const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
        const QColor &boardDefaultColor) {
    // 1. NodeRect's own color
    if (nodeRectColor.isValid())
        return nodeRectColor;

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
        const int cardId, const Card &cardData, const QRectF &rect, const QColor &color,
        const QStringList &userLabelsList) {
    Q_ASSERT(!cardIdToNodeRect.contains(cardId));

    auto *nodeRect = new NodeRect(cardId);
    cardIdToNodeRect.insert(cardId, nodeRect);
    boardView->graphicsScene->addItem(nodeRect);
    nodeRect->setZValue(zValueForNodeRects);
    nodeRect->initialize();

    nodeRect->setNodeLabels(
            sortByOrdering(cardData.getLabels(), userLabelsList, false));
    nodeRect->setTitle(cardData.title);
    nodeRect->setText(cardData.text);

    nodeRect->setRect(rect);
    nodeRect->setColor(color);

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

        //
        boardView->adjustSceneRect();
    });

    QObject::connect(nodeRect, &NodeRect::finishedMovingOrResizing,
                     boardView, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        //
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

        for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
            it.value()->setHighlighted(it.key() == cardIdClicked);

        //
        highlightedCardId = cardIdClicked;

        // call AppData
        Services::instance()->getAppData()
                ->setHighlightedCardId(EventSource(boardView), highlightedCardId);
    });

    QObject::connect(nodeRect, &NodeRect::titleTextUpdated,
            boardView,
            [this, nodeRectPtr](const StringOpt &updatedTitle, const StringOpt &updatedText) {
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
        cardIdToNodeRect.value(highlightedCardId)->setHighlighted(false);

        highlightedCardId = -1;
        *highlightedCardIdChanged = true;
    }
}

void BoardView::NodeRectsCollection::updateAllNodeRectColors() {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        NodeRect *nodeRect = it.value();

        const QColor nodeRectOwnColor {}; // [temp]
        const QColor color = computeNodeRectColor(
                nodeRectOwnColor, nodeRect->getNodeLabels(),
                boardView->cardLabelsAndAssociatedColors, boardView->defaultNodeRectColor);

        nodeRect->setColor(color);
    }
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

QSet<NodeRect *> BoardView::NodeRectsCollection::getAllNodeRects() const {
    QSet<NodeRect *> result;
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        result << it.value();
    return result;
}

//====

EdgeArrow *BoardView::EdgeArrowsCollection::createEdgeArrow(
        const RelationshipId relId, const EdgeArrowData &edgeArrowData) {
    Q_ASSERT(!relIdToEdgeArrow.contains(relId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.startCardId));
    Q_ASSERT(boardView->nodeRectsCollection.contains(relId.endCardId));

    auto *edgeArrow = new EdgeArrow(relId);
    relIdToEdgeArrow.insert(relId, edgeArrow);
    cardIdPairToParallelRels[QSet<int> {relId.startCardId, relId.endCardId}] << relId;

    boardView->graphicsScene->addItem(edgeArrow);
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
