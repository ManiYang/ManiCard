#include "board_view.h"

BoardView::BoardView(QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet(styleSheet());
}

QString BoardView::styleSheet() {
    return
        "BoardView {"
        "  background: white;"
        "}";
}
