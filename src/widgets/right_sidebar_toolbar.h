#ifndef RIGHTSIDEBARTOOLBAR_H
#define RIGHTSIDEBARTOOLBAR_H

#include <QAbstractButton>
#include <QFrame>
#include "widgets/icons.h"
#include "widgets/components/simple_toolbar.h"

class RightSidebarToolBar : public SimpleToolBar
{
    Q_OBJECT
public:
    explicit RightSidebarToolBar(QWidget *parent = nullptr);

signals:
    void closeRightSidebar();

private:
    QHash<QAbstractButton *, Icon> buttonToIcon;

    void setUpButtonsWithIcons();
};

#endif // RIGHTSIDEBARTOOLBAR_H
