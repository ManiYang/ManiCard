#include <QDebug>
#include <QGraphicsView>
#include <QInputDialog>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "board_view.h"
#include "cached_data_access.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "utilities/strings_util.h"
#include "widgets/components/edge_arrow.h"
#include "widgets/components/graphics_scene.h"
#include "widgets/components/node_rect.h"
#include "widgets/dialogs/dialog_create_relationship.h"

using StringOpt = std::optional<QString>;
using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

BoardView::BoardView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpContextMenu();
    setUpConnections();
    installEventFiltersOnComponents();
}

void BoardView::loadBoard(const int boardIdToLoad, std::function<void (bool)> callback) {
    if (boardId == boardIdToLoad) {
        callback(true);
        return;
    }

    // close all cards
    if (boardId != -1) {
        if (!canClose()) {
            callback(false);
            return;
        }

        closeAllCards();
        boardId = -1;
    }

    if (boardIdToLoad == -1) {
        callback(true);
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Board board;
        QHash<int, Card> cardsData;
        QHash<RelationshipId, RelationshipProperties> relationshipsData;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, boardIdToLoad]() {
        // 1. get board data
        Services::instance()->getCachedDataAccess()->getBoardData(
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
        Services::instance()->getCachedDataAccess()->queryCards(
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

    routine->addStep([this, routine, boardIdToLoad]() {
        // 3. open cards, creating NodeRect's
        ContinuationContext context(routine);

        for (auto it = routine->cardsData.constBegin(); it != routine->cardsData.constEnd(); ++it) {
            const int &cardId = it.key();
            const Card &cardData = it.value();

            const NodeRectData nodeRectData = routine->board.cardIdToNodeRectData.value(cardId);

            constexpr bool saveNodeRectData = false;
            NodeRect *nodeRect = createNodeRect(cardId, cardData, nodeRectData, saveNodeRectData);
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

        Services::instance()->getCachedDataAccess()->queryRelationshipsFromToCards(
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
            createEdgeArrow(relId, edgeArrowData);
    }, this);

    routine->addStep([this, routine, callback, boardIdToLoad]() {
        // final step
        ContinuationContext context(routine);

        if (!routine->errorFlag)
            boardId = boardIdToLoad;

        callback(!routine->errorFlag);
    }, this);

    routine->start();
}

void BoardView::prepareToClose() {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        it.value()->prepareToClose();
}

int BoardView::getBoardId() const {
    return boardId;
}

QPointF BoardView::getViewTopLeftPos() const {
    return graphicsView->mapToScene(0, 0);
}

bool BoardView::canClose() const {
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it) {
        if (!it.value()->canClose())
            return false;
    }
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
    {
        auto *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout);

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
            userToOpenExistingCard(contextMenuData.requestScenePos);
        });
    }
    {
        QAction *action = contextMenu->addAction(
                QIcon(":/icons/add_box_black_24"), "Create New Card");
        connect(action, &QAction::triggered, this, [this]() {
            userToCreateNewCard(contextMenuData.requestScenePos);
        });
    }
}

void BoardView::setUpConnections() {
    connect(graphicsScene, &GraphicsScene::contextMenuRequestedOnScene,
            this, [this](const QPointF &scenePos) {
        contextMenuData.requestScenePos = scenePos;
        contextMenu->popup(getScreenPosFromScenePos(scenePos));
    });
}

void BoardView::installEventFiltersOnComponents() {
    graphicsView->installEventFilter(this);
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

void BoardView::userToOpenExistingCard(const QPointF &scenePos) {
    constexpr int minValue = 0;
    constexpr int step = 1;

    bool ok;
    const int cardId = QInputDialog::getInt(
            this, "Open Card", "Card ID to open:", 0, minValue, INT_MAX, step, &ok);
    if (!ok)
        return;

    // check already opened
    if (cardIdToNodeRect.contains(cardId)) {
        QMessageBox::information(this, " ", QString("Card %1 already opened.").arg(cardId));
        return;
    }

    //
    openExistingCard(cardId, scenePos);
}

void BoardView::userToCreateNewCard(const QPointF &scenePos) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newCardId;
        Card card;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([routine]() {
        // 1. request new card ID
        Services::instance()->getCachedDataAccess()->requestNewCardId(
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

        NodeRectData nodeRectData;
        {
            nodeRectData.rect = QRectF(scenePos, defaultNewNodeRectSize);
            nodeRectData.color = defaultNewNodeRectColor;
        }
        constexpr bool saveNodeRectData = true;
        NodeRect *nodeRect = createNodeRect(
                routine->newCardId, routine->card, nodeRectData, saveNodeRectData);
        nodeRect->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine]() {
        // 3. write DB
        Services::instance()->getCachedDataAccess()->createNewCardWithId(
                routine->newCardId, routine->card,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Could not save created card to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 4. (final step)
        ContinuationContext context(routine);

        if (routine->errorFlag)
            createWarningMessageBox(this, " ", routine->errorMsg)->exec();

    }, this);

    routine->start();
}

void BoardView::userToCreateRelationship(const int cardId) {
    Q_ASSERT(cardIdToNodeRect.contains(cardId));

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        RelationshipId relIdToCreate {-1, -1, ""};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine, cardId]() {
        // show dialog, get relationship to create
        auto *dialog = new DialogCreateRelationship(
                cardId, cardIdToNodeRect.value(cardId)->getTitle(), this);

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
        Services::instance()->getCachedDataAccess()->queryCards(
                startEndCards,
                // callback
                [this, routine, startEndCards](bool ok, QHash<int, Card> cards) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        QMessageBox::warning(
                                this, " ", "Could not query start/end cards. See logs for detail");
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
                        QMessageBox::warning(
                                this, " ", QString("Card %1 not found.").arg(cardIdsStr));
                        context.setErrorFlag();
                        return;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // create relationship
        Services::instance()->getCachedDataAccess()->createRelationship(
                routine->relIdToCreate,
                // callback
                [this, routine](bool ok, bool created) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        // (don't set routine->errorFlag here)
                        const auto msg
                                = QString("Could not save created relationship to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();
                        return;
                    }
                    if (!created) {
                        QMessageBox::information(
                                this, " ",
                                QString("Relationship %1 already exists.")
                                    .arg(routine->relIdToCreate.toString()));
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // create EdgeArrow
        ContinuationContext context(routine);

        EdgeArrowData edgeArrowData;
        {
            edgeArrowData.lineColor = defaultEdgeArrowLineColor;
            edgeArrowData.lineWidth = defaultEdgeArrowLineWidth;
        }
        createEdgeArrow(routine->relIdToCreate, edgeArrowData);
    }, this);

    routine->addStep([routine]() {
        // final step
        routine->nextStep();
    }, this);

    routine->start();
}

void BoardView::userToCloseNodeRect(const int cardId) {
    Q_ASSERT(cardIdToNodeRect.contains(cardId));

    auto *routine = new AsyncRoutine;

    //
    routine->addStep([this, routine, cardId]() {
        NodeRect *nodeRect = cardIdToNodeRect.value(cardId);
        nodeRect->prepareToClose();

        // wait until nodeRect->canClose() returns true
        (new PeriodicChecker)->setPeriod(50)->setTimeOut(20000)
            ->setPredicate([nodeRect]() {
                return nodeRect->canClose();
            })
            ->onPredicateReturnsTrue([routine]() {
                routine->nextStep();
            })
            ->onTimeOut([routine, cardId]() {
                qWarning().noquote()
                        << QString("time-out while awaiting NodeRect::canClose() for card %1")
                           .arg(cardId);
                routine->nextStep();
            })
            ->setAutoDelete()->start();
    }, this);

    routine->addStep([this, routine, cardId]() {
        constexpr bool removeConnectedEdgeArrows = true;
        closeNodeRect(cardId, removeConnectedEdgeArrows);
        routine->nextStep();
    }, this);

    routine->addStep([this, routine, cardId]() {
        // write DB
        Services::instance()->getCachedDataAccess()->removeNodeRect(
                boardId, cardId,
                // callback
                [this, routine](bool ok) {
                    if (!ok) {
                        const auto msg
                                = QString("Could not remove NodeRect from DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();
                    }
                    routine->nextStep();
                },
                this
        );
    }, this);

    routine->start();
}

void BoardView::openExistingCard(const int cardId, const QPointF &scenePos) {
    Services::instance()->getCachedDataAccess()->queryCards(
            {cardId},
            // callback
            [=](bool ok, const QHash<int, Card> &cards) {
                if (!ok) {
                    createWarningMessageBox(
                            this, " ", "Could not open card. See logs for details.")->exec();
                    return;
                }

                if (!cards.contains(cardId)) {
                    createInformationMessageBox(
                            this, " ", QString("Card %1 not found.").arg(cardId))->exec();
                    return;
                }

                const Card &cardData = cards.value(cardId);

                NodeRectData nodeRectData;
                {
                    nodeRectData.rect = QRectF(scenePos, defaultNewNodeRectSize);
                    nodeRectData.color = defaultNewNodeRectColor;
                }
                constexpr bool saveNodeRectData = true;
                auto *nodeRect = createNodeRect(cardId, cardData, nodeRectData, saveNodeRectData);
                nodeRect->setEditable(true);

                adjustSceneRect();
            },
            this
    );
}

void BoardView::saveCardPropertiesUpdate(
        NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
        std::function<void ()> callback) {
    Services::instance()->getCachedDataAccess()->updateCardProperties(
            nodeRect->getCardId(), propertiesUpdate,
            // callback
            [this, callback](bool ok) {
                if (!ok) {
                    const auto msg
                            = QString("Could not save card properties to DB.\n\n"
                                      "There is unsaved update. See %1")
                              .arg(Services::instance()->getUnsavedUpdateFilePath());
                    createWarningMessageBox(this, " ", msg)->exec();
                }

                if (callback)
                    callback();
            },
            this
    );
}

void BoardView::closeAllCards() {
    const QSet<int> cardIds = keySet(cardIdToNodeRect);
    for (const int &cardId: cardIds) {
        constexpr bool removeConnectedEdgeArrows = false;
        closeNodeRect(cardId, removeConnectedEdgeArrows);
    }

    const auto relIds = keySet(relIdToEdgeArrow);
    removeEdgeArrows(relIds);
}

NodeRect *BoardView::createNodeRect(
        const int cardId, const Card &cardData,
        const NodeRectData &nodeRectData, const bool saveCreatedNodeRectData) {
    Q_ASSERT(!cardIdToNodeRect.contains(cardId));

    auto *nodeRect = new NodeRect(cardId);
    cardIdToNodeRect.insert(cardId, nodeRect);
    graphicsScene->addItem(nodeRect);
    nodeRect->setZValue(zValueForNodeRects);
    nodeRect->initialize();

    nodeRect->setNodeLabels(cardData.getLabels());
    nodeRect->setTitle(cardData.title);
    nodeRect->setText(cardData.text);

    nodeRect->setRect(nodeRectData.rect);
    nodeRect->setColor(nodeRectData.color);

    // set up connections
    QPointer<NodeRect> nodeRectPtr(nodeRect);
    connect(nodeRect, &NodeRect::movedOrResized, this, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        // update edge arrows
        const QSet<RelationshipId> relIds
                = getEdgeArrowsConnectingNodeRect(nodeRectPtr->getCardId());
        for (const auto &relId: relIds)
            updateEdgeArrow(relId);
    });

    connect(nodeRect, &NodeRect::finishedMovingOrResizing, this, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;

        // save
        NodeRectDataUpdate update;
        update.rect = nodeRectPtr->getRect();

        Services::instance()->getCachedDataAccess()->updateNodeRectProperties(
                boardId, nodeRectPtr->getCardId(), update,
                // callback
                [this](bool ok) {
                    if (!ok) {
                        const auto msg
                                = QString("Could not save NodeRect's rect to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();
                    }
                },
                this
        );
    });

    connect(nodeRect, &NodeRect::saveTitleTextUpdate,
            this,
            [this, nodeRectPtr](const StringOpt &updatedTitle, const StringOpt &updatedText) {
        if (!nodeRectPtr)
            return;

        CardPropertiesUpdate propertiesUpdate;
        {
            propertiesUpdate.title = updatedTitle;
            propertiesUpdate.text = updatedText;
        }
        saveCardPropertiesUpdate(
                nodeRectPtr.data(), propertiesUpdate,
                // callback:
                [nodeRectPtr]() {
                    if (nodeRectPtr)
                        nodeRectPtr->finishedSaveTitleText();
                }
        );
    });

    connect(nodeRect, &NodeRect::userToCreateRelationship, this, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        userToCreateRelationship(nodeRectPtr->getCardId());
    });

    connect(nodeRect, &NodeRect::closeByUser, this, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        userToCloseNodeRect(nodeRectPtr->getCardId());
    });

    //
    if (saveCreatedNodeRectData) {
        // save the created NodeRect
        Services::instance()->getCachedDataAccess()->createNodeRect(
                boardId, cardId, nodeRectData,
                // callback
                [this](bool ok) {
                    if (!ok) {
                        const auto msg
                                = QString("Could not save created NodeRect to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();
                    }
                },
                this
        );
    }

    //
    return nodeRect;
}

void BoardView::closeNodeRect(const int cardId, const bool removeConnectedEdgeArrows) {
    NodeRect *nodeRect = cardIdToNodeRect.take(cardId);
    if (nodeRect == nullptr)
        return;

    graphicsScene->removeItem(nodeRect);
    nodeRect->deleteLater();

    if (removeConnectedEdgeArrows) {
        const auto relIds = getEdgeArrowsConnectingNodeRect(cardId);
        removeEdgeArrows(relIds);
    }

    //
    graphicsScene->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
    // this is to deal with the QGraphicsView problem
    // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug
}

EdgeArrow *BoardView::createEdgeArrow(
        const RelationshipId relId, const EdgeArrowData &edgeArrowData) {
    Q_ASSERT(!relIdToEdgeArrow.contains(relId));
    Q_ASSERT(cardIdToNodeRect.contains(relId.startCardId));
    Q_ASSERT(cardIdToNodeRect.contains(relId.endCardId));

    auto *edgeArrow = new EdgeArrow(relId);
    relIdToEdgeArrow.insert(relId, edgeArrow);
    graphicsScene->addItem(edgeArrow);
    edgeArrow->setZValue(zValueForEdgeArrows);

    updateEdgeArrow(relId);

    edgeArrow->setLineWidth(edgeArrowData.lineWidth);
    edgeArrow->setLineColor(edgeArrowData.lineColor);

    //
    return edgeArrow;
}

void BoardView::updateEdgeArrow(const RelationshipId relId) {
    Q_ASSERT(relIdToEdgeArrow.contains(relId));
    auto *edgeArrow = relIdToEdgeArrow.value(relId);

    const QLineF line = computeEdgeArrowLine(
            cardIdToNodeRect.value(relId.startCardId)->getRect(),
            cardIdToNodeRect.value(relId.endCardId)->getRect());
    edgeArrow->setStartEndPoint(line.p1(), line.p2());

    edgeArrow->setLabel(relId.type);
}

void BoardView::removeEdgeArrows(const QSet<RelationshipId> &relIds) {
    for (const auto &relId: relIds) {
        if (!relIdToEdgeArrow.contains(relId))
            continue;

        EdgeArrow *edgeArrow = relIdToEdgeArrow.take(relId);
        graphicsScene->removeItem(edgeArrow);
        delete edgeArrow;
    }
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

QSet<RelationshipId> BoardView::getEdgeArrowsConnectingNodeRect(const int cardId) {
    QSet<RelationshipId> result;

    const auto relIds = keySet(relIdToEdgeArrow);
    for (const RelationshipId &relId: relIds) {
        if (relId.connectsCard(cardId))
            result << relId;
    }

    return result;
}

namespace  {
bool rectEdgeIntersectsWithLine(
        const QRectF &rect, const QLineF &line, QPointF *intersectionPoint);
}

QLineF BoardView::computeEdgeArrowLine(
        const QRectF &startNodeRect, const QRectF &endNodeRect) {
    QLineF lineC2C(startNodeRect.center(), endNodeRect.center());

    QPointF intersectionPoint;
    bool intersect = rectEdgeIntersectsWithLine(startNodeRect, lineC2C, &intersectionPoint);
    const QPointF startPoint = intersect ?  intersectionPoint : startNodeRect.center();

    intersect = rectEdgeIntersectsWithLine(endNodeRect, lineC2C, &intersectionPoint);
    const QPointF endPoint = intersect ?  intersectionPoint : endNodeRect.center();

    return {startPoint, endPoint};
}

namespace  {
bool rectEdgeIntersectsWithLine(
        const QRectF &rect, const QLineF &line, QPointF *intersectionPoint) {
    const QVector<QLineF> edges {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())
    };
    for (const QLineF &edge: edges) {
        const auto intersectType = edge.intersects(line, intersectionPoint);
        if (intersectType == QLineF::BoundedIntersection) {
            return true;
        }
    }
    return false;
}
} // namespace
