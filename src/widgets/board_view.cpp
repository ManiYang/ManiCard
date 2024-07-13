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
#include "widgets/components/graphics_scene.h"
#include "widgets/components/node_rect.h"

#include <QGraphicsRectItem>
#include <QTimer>

using StringOpt = std::optional<QString>;

BoardView::BoardView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpContextMenu();
    setUpConnections();
    installEventFiltersOnComponents();

    // test...
//    {
//        auto *rect1 = new QGraphicsRectItem(0, 0, 100, 200); // x,y,w,h
//        rect1->setPen({QBrush(Qt::red), 1.0});
//        graphicsScene->addItem(rect1);
//    }
//    {
//        auto *rect2 = new QGraphicsRectItem(160, 100, 200, 100); // x,y,w,h
//        graphicsScene->addItem(rect2);
    //    }
}

bool BoardView::canClose() const {
    return false; // [temp]....
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
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, boardIdToLoad]() {
        // 1. get board data
        Services::instance()->getCachedDataAccess()->getBoardData(
                boardIdToLoad,
                // callback
                [routine](bool ok, std::optional<Board> board) {
                    auto context = routine->continuationContext();

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
                    auto context = routine->continuationContext();

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
        // 3. open cards
        auto context = routine->continuationContext();

        for (auto it = routine->cardsData.constBegin(); it != routine->cardsData.constEnd(); ++it) {
            const int &cardId = it.key();
            const Card &cardData = it.value();

            const NodeRectData nodeRectData = routine->board.cardIdToNodeRectData.value(cardId);

            NodeRect *nodeRect = createNodeRect(cardId, cardData);
            nodeRect->setRect(nodeRectData.rect);
            nodeRect->setColor(nodeRectData.color);
            nodeRect->setEditable(true);
        }

        //
        boardId = boardIdToLoad;
        adjustSceneRect();
        setViewTopLeftPos(routine->board.topLeftPos);
    }, this);

    routine->addStep([routine, callback]() {
        // 4. (final step)
        auto context = routine->continuationContext();
        callback(!routine->errorFlag);
    }, this);

    routine->start();
}

int BoardView::getBoardId() const {
    return boardId;
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

                auto *nodeRect = createNodeRect(cardId, cardData);
                nodeRect->setRect(QRectF(scenePos, defaultNewNodeRectSize));
                nodeRect->setEditable(true);

                adjustSceneRect();
            },
            this
    );
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
                    auto context = routine->continuationContext();
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
        auto context = routine->continuationContext();

        routine->card.title = "New Card";
        routine->card.text = "";

        NodeRect *nodeRect = createNodeRect(routine->newCardId, routine->card);
        nodeRect->setRect(QRectF(scenePos, defaultNewNodeRectSize));
        nodeRect->setEditable(true);

        adjustSceneRect();
    }, this);

    routine->addStep([this, routine]() {
        // 3. write DB
        Services::instance()->getCachedDataAccess()->createNewCardWithId(
                routine->newCardId, routine->card,
                // callback
                [routine](bool ok) {
                    auto context = routine->continuationContext();

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
        auto context = routine->continuationContext();

        if (routine->errorFlag)
            createWarningMessageBox(this, " ", routine->errorMsg)->exec();

    }, this);

    routine->start();
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
    for (auto it = cardIdToNodeRect.constBegin(); it != cardIdToNodeRect.constEnd(); ++it)
        closeNodeRect(it.key());
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

NodeRect *BoardView::createNodeRect(const int cardId, const Card &cardData) {
    Q_ASSERT(!cardIdToNodeRect.contains(cardId));

    auto *nodeRect = new NodeRect(cardId);
    cardIdToNodeRect.insert(cardId, nodeRect);
    graphicsScene->addItem(nodeRect);
    nodeRect->initialize();

    nodeRect->setNodeLabels(cardData.getLabels());
    nodeRect->setTitle(cardData.title);
    nodeRect->setText(cardData.text);

    QPointer<NodeRect> nodeRectPtr(nodeRect);
    connect(nodeRect, &NodeRect::savePropertiesUpdate,
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
                        nodeRectPtr->finishedSavePropertiesUpdate();
                }
        );
    });

    connect(nodeRect, &NodeRect::closeByUser, this, [this, nodeRectPtr]() {
        if (!nodeRectPtr)
            return;
        closeNodeRect(nodeRectPtr->getCardId());
    });

    return nodeRect;
}

void BoardView::closeNodeRect(const int cardId) {
    NodeRect *nodeRect = cardIdToNodeRect.take(cardId);
    if (nodeRect == nullptr)
        return;
    graphicsScene->removeItem(nodeRect);
    nodeRect->deleteLater();
}
