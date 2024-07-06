#include <QGraphicsView>
#include <QVBoxLayout>
#include "board_view.h"

BoardView::BoardView(QWidget *parent)
    : QFrame(parent)
{
    setUpWidgets();
    setStyleSheet(styleSheet());
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
    graphicsView->setFrameShape(QFrame::NoFrame);


}

QString BoardView::styleSheet() {
    return
        "BoardView {"
        "}";
}
