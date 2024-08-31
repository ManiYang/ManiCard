#include "simple_toolbar.h"
#include "utilities/margins_util.h"

constexpr int toolBarHeight = 32;
constexpr int toolBarPadding = 2;

SimpleToolBar::SimpleToolBar(QWidget *parent)
        : QFrame(parent)
        , hLayout(new QHBoxLayout) {
    setFrameShape(QFrame::NoFrame);
    setFixedHeight(toolBarHeight);

    setLayout(hLayout);
    hLayout->setContentsMargins(uniformMargins(toolBarPadding));

    setStyleSheet(
            "SimpleToolBar {"
            "  background: white;"
            "}"
            "SimpleToolBar > QToolButton {"
            "  border: none;"
            "  background: transparent;"
            "}"
            "SimpleToolBar > QToolButton:hover {"
            "  background: #e0e0e0;"
            "}");
}
