#include <QDebug>
#include <QGraphicsView>
#include <QInputDialog>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "board_view.h"
#include "cached_data_access.h"
#include "services.h"
#include "widgets/components/graphics_scene.h"

#include <QGraphicsRectItem>
#include <QTimer>
#include "widgets/components/node_rect.h"

BoardView::BoardView(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpContextMenu();
    setUpConnections();
    installEventFiltersOnComponents();
    setStyleSheet(styleSheet());

    // test...
//    {
//        auto *rect1 = new QGraphicsRectItem(0, 0, 100, 200); // x,y,w,h
//        rect1->setPen({QBrush(Qt::red), 1.0});
//        graphicsScene->addItem(rect1);
//    }
    {
        auto *rect2 = new QGraphicsRectItem(160, 100, 200, 100); // x,y,w,h
        graphicsScene->addItem(rect2);
    }
    {
        auto *nodeRect = new NodeRect;
        graphicsScene->addItem(nodeRect);
        nodeRect->initialize();

        nodeRect->setRect({0, 0, 150, 200}); // x,y,w,h
        nodeRect->setNodeLabel(":Test");
        nodeRect->setCardId(123);
        nodeRect->setTitle("Test Card with Long Card Title");
        nodeRect->setText(
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
                "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");
        nodeRect->setEditable(true);


    }



}

bool BoardView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == graphicsView) {
        if (event->type() == QEvent::Resize)
            onGraphicsViewResize();
    }
    return false;
}

void BoardView::showEvent(QShowEvent */*event*/) {
    if (!isEverShown) {
        isEverShown = true;
        onShownForFirstTime();
    }
}

void BoardView::setUpWidgets() {
    const QColor sceneBackgroundColor(235, 235, 235);

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

QString BoardView::styleSheet() {
    return
        "BoardView {"
        "}";
}

void BoardView::onShownForFirstTime() {
    // adjust view's center to the center of contents
    {
        const auto contentsCenter = graphicsScene->itemsBoundingRect().center();
        graphicsView->centerOn(contentsCenter.isNull() ? QPointF(0, 0) : contentsCenter);
    }
}

void BoardView::onGraphicsViewResize() {
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

//    //
//    if (mCardIdToNodeRectId.contains(cardId))
//    {
//        QMessageBox::information(this, "info", QString("Card %1 already opened.").arg(cardId));
//        return;
//    }

    //
    openExistingCard(cardId, scenePos);
}

void BoardView::openExistingCard(const int cardId, const QPointF &scenePos) {
    Services::instance()->getCachedDataAccess()->queryCards(
            {cardId},
            // callback
            [this, cardId](bool ok, const QHash<int, Card> &cards) {
                if (!cards.contains(cardId)) {
                    // card not found ...


                }
                else {
                    const Card &cardData = cards.value(cardId);

                    // create a NodeRect that display `cardData` ....





                }
            },
            this
    );
}

QPoint BoardView::getScreenPosFromScenePos(const QPointF &scenePos) {

    QPoint posInViewport = graphicsView->mapFromScene(scenePos);
    return graphicsView->viewport()->mapToGlobal(posInViewport);
}
