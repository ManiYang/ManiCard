#ifndef RIGHTSIDEBAR_H
#define RIGHTSIDEBAR_H

#include <QFrame>

class RightSidebarToolBar;

class RightSidebar : public QFrame
{
    Q_OBJECT
public:
    explicit RightSidebar(QWidget *parent = nullptr);

signals:
    void closeRightSidebar();

private:
    RightSidebarToolBar *toolBar {nullptr};

    void setUpWidgets();
    void setUpConnections();
};

#endif // RIGHTSIDEBAR_H
