#include <QHBoxLayout>
#include <QToolButton>
#include "right_sidebar_toolbar.h"

RightSidebarToolBar::RightSidebarToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    hLayout->addStretch();

    {
        auto *button = new QToolButton;
        hLayout->addWidget(button);
        hLayout->setAlignment(button, Qt::AlignVCenter);

        button->setIcon(QIcon(":/icons/close_right_panel_24"));
        button->setIconSize({24, 24});
        connect(button, &QToolButton::clicked, this, [this]() {
            emit closeRightSidebar();
        });
    }
}
