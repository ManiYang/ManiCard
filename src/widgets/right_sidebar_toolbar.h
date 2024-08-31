#ifndef RIGHTSIDEBARTOOLBAR_H
#define RIGHTSIDEBARTOOLBAR_H

#include <QFrame>
#include "widgets/components/simple_toolbar.h"

class RightSidebarToolBar : public SimpleToolBar
{
    Q_OBJECT
public:
    explicit RightSidebarToolBar(QWidget *parent = nullptr);

signals:
    void closeRightSidebar();
};

#endif // RIGHTSIDEBARTOOLBAR_H
