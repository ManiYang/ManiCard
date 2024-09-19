#include <QDebug>
#include <QHBoxLayout>
#include "board_view_toolbar.h"

BoardViewToolBar::BoardViewToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    setUpBoardSettingsMenu();

    //
    hLayout->addStretch();
    {
        buttonBoardSettings = new QToolButton;
        hLayout->addWidget(buttonBoardSettings);
        hLayout->setAlignment(buttonBoardSettings, Qt::AlignVCenter);

        buttonBoardSettings->setIcon(QIcon(":/icons/more_vert_24"));
        buttonBoardSettings->setIconSize({24, 24});
        buttonBoardSettings->setToolTip("Board Settings");
        connect(buttonBoardSettings, &QToolButton::clicked, this, [this]() {
            const QSize s = buttonBoardSettings->size();
            const QPoint bottomRight = buttonBoardSettings->mapToGlobal({s.width(), s.height()});
            boardSettingsMenu->popup({
                bottomRight.x() - boardSettingsMenu->sizeHint().width(),
                bottomRight.y()
            });
        });
    }
    {
        buttonOpenRightSidebar = new QToolButton;
        hLayout->addWidget(buttonOpenRightSidebar);
        hLayout->setAlignment(buttonOpenRightSidebar, Qt::AlignVCenter);

        buttonOpenRightSidebar->setIcon(QIcon(":/icons/open_right_panel_24"));
        buttonOpenRightSidebar->setIconSize({24, 24});
        buttonOpenRightSidebar->setToolTip("Open Right Side-Bar");
        connect(buttonOpenRightSidebar, &QToolButton::clicked, this, [this]() {
            emit openRightSidebar();
            buttonOpenRightSidebar->setVisible(false);
        });
    }
}

void BoardViewToolBar::showButtonOpenRightSidebar() {
    buttonOpenRightSidebar->setVisible(true);
}

void BoardViewToolBar::setUpBoardSettingsMenu() {
    boardSettingsMenu = new QMenu(this);

    {
        boardSettingsMenu->addAction("Card Colors...", this, [this]() {
            emit openCardColorsDialog();
        });
    }
}

void BoardViewToolBar::setUpConnections() {
    connect(boardSettingsMenu, &QMenu::aboutToHide, this, [this]() {
        buttonBoardSettings->update();
                // without this, the button's appearence stay in hover state
    });
}
