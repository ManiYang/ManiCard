#ifndef RIGHTSIDEBAR_H
#define RIGHTSIDEBAR_H

#include <QFrame>
#include <QStackedWidget>

class CardPropertiesView;
class RightSidebarToolBar;

class RightSidebar : public QFrame
{
    Q_OBJECT
public:
    explicit RightSidebar(QWidget *parent = nullptr);

    void loadCardProperties(const int cardId); // `cardId` can be -1

signals:
    void closeRightSidebar();

private:
    RightSidebarToolBar *toolBar {nullptr};
    QStackedWidget *stackedWidget {nullptr};
    CardPropertiesView *cardPropertiesView {nullptr};

    void setUpWidgets();
    void setUpConnections();
};

#endif // RIGHTSIDEBAR_H
