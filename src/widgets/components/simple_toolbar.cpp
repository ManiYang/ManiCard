#include "simple_toolbar.h"
#include "utilities/margins_util.h"
#include "widgets/app_style_sheet.h"

constexpr int toolBarHeight = 32;
constexpr int toolBarPadding = 2;

SimpleToolBar::SimpleToolBar(QWidget *parent)
        : QFrame(parent)
        , hLayout(new QHBoxLayout) {
    setFrameShape(QFrame::NoFrame);
    setFixedHeight(toolBarHeight);

    setLayout(hLayout);
    hLayout->setContentsMargins(uniformMargins(toolBarPadding));

    setStyleClasses(this, {StyleClass::highContrastBackground});
}
