#include <QVBoxLayout>
#include "right_sidebar.h"
#include "widgets/right_sidebar_toolbar.h"

RightSidebar::RightSidebar(QWidget *parent)
        : QFrame(parent){
    setUpWidgets();
    setUpConnections();
}

void RightSidebar::setUpWidgets() {

    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    {
        toolBar = new RightSidebarToolBar;
        layout->addWidget(toolBar);

        //
        layout->addStretch();
    }
}

void RightSidebar::setUpConnections() {
    connect(toolBar, &RightSidebarToolBar::closeRightSidebar, this, [this]() {
        emit closeRightSidebar();
    });
}
