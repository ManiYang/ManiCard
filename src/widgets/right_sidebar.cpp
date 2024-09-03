#include <QDebug>
#include <QVBoxLayout>
#include "right_sidebar.h"
#include "widgets/card_properties_view.h"
#include "widgets/right_sidebar_toolbar.h"

RightSidebar::RightSidebar(QWidget *parent)
        : QFrame(parent){
    setUpWidgets();
    setUpConnections();
}

void RightSidebar::loadCardProperties(const int cardId) {
    cardPropertiesView->loadCard(cardId);
}

void RightSidebar::setUpWidgets() {
    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    {
        toolBar = new RightSidebarToolBar;
        layout->addWidget(toolBar);

        //
        stackedWidget = new QStackedWidget;
        layout->addWidget(stackedWidget);
        {
            cardPropertiesView = new CardPropertiesView;
            stackedWidget->addWidget(cardPropertiesView);
        }

        stackedWidget->setCurrentWidget(cardPropertiesView);
    }

    //
    setStyleSheet(
            "RightSidebar {"
            "  background: white;"
            "}");
}

void RightSidebar::setUpConnections() {
    connect(toolBar, &RightSidebarToolBar::closeRightSidebar, this, [this]() {
        emit closeRightSidebar();
    });
}
