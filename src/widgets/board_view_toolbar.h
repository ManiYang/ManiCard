#ifndef BOARDVIEWTOOLBAR_H
#define BOARDVIEWTOOLBAR_H

#include <QFrame>
#include <QMenu>
#include <QToolButton>
#include "widgets/components/simple_toolbar.h"

class BoardViewToolBar : public SimpleToolBar
{
    Q_OBJECT
public:
    explicit BoardViewToolBar(QWidget *parent = nullptr);

    void showButtonOpenRightSidebar();

signals:
    void openRightSidebar();
    void openCardColorsDialog();

private:
    QToolButton *buttonOpenRightSidebar {nullptr};
    QToolButton *buttonBoardSettings {nullptr};
    QMenu *boardSettingsMenu {nullptr};

    void setUpBoardSettingsMenu();
    void setUpConnections();
};

#endif // BOARDVIEWTOOLBAR_H
