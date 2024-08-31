#include <QHBoxLayout>
#include "board_view_toolbar.h"

BoardViewToolBar::BoardViewToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    hLayout->addStretch();

    {
        buttonOpenRightSidebar = new QToolButton;
        hLayout->addWidget(buttonOpenRightSidebar);
        hLayout->setAlignment(buttonOpenRightSidebar, Qt::AlignVCenter);

        buttonOpenRightSidebar->setIcon(QIcon(":/icons/open_right_panel_24"));
        buttonOpenRightSidebar->setIconSize({24, 24});
        connect(buttonOpenRightSidebar, &QToolButton::clicked, this, [this]() {
            emit openRightSidebar();
            buttonOpenRightSidebar->setVisible(false);
        });
    }
}

void BoardViewToolBar::showButtonOpenRightSidebar() {
    buttonOpenRightSidebar->setVisible(true);
}
