#include <QDebug>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "board_view.h"
#include "widgets/components/graphics_scene.h"

#include <QGraphicsRectItem>
#include <QTimer>
#include "widgets/components/node_rect.h"

BoardView::BoardView(QWidget *parent)
    : QFrame(parent)
{
    setUpWidgets();
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

        nodeRect->setRect({0, 0, 150, 200}); // x,y,w,h
        nodeRect->setNodeLabel(":Test");
        nodeRect->setCardId(123);

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
    graphicsView->setScene(graphicsScene);

    graphicsView->setRenderHint(QPainter::Antialiasing, true);
    graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);

    graphicsView->setFrameShape(QFrame::NoFrame);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
