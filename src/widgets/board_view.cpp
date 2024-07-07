#include <QDebug>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "board_view.h"
#include "widgets/components/graphics_scene.h"

#include <QGraphicsRectItem>
#include <QTimer>

BoardView::BoardView(QWidget *parent)
    : QFrame(parent)
{
    setUpWidgets();
    installEventFilters();
    setStyleSheet(styleSheet());


    // test...
    QTimer::singleShot(1000, this, [this]() {
        auto *rect1 = new QGraphicsRectItem(0, 0, 100, 200); // x,y,w,h
        graphicsScene->addItem(rect1);

        auto *rect2 = new QGraphicsRectItem(200, 100, 200, 80); // x,y,w,h
        graphicsScene->addItem(rect2);

        onGraphicsViewResize();
    });


}

bool BoardView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == graphicsView) {
        if (event->type() == QEvent::Resize)
            onGraphicsViewResize();
    }
    return false;
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

    graphicsView->setFrameShape(QFrame::NoFrame);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void BoardView::installEventFilters() {
    graphicsView->installEventFilter(this);
}

QString BoardView::styleSheet() {
    return
        "BoardView {"
        "}";
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
