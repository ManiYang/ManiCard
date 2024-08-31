#ifndef BOARDVIEWTOOLBAR_H
#define BOARDVIEWTOOLBAR_H

#include <QFrame>
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

private:
    QToolButton *buttonOpenRightSidebar {nullptr};
};

#endif // BOARDVIEWTOOLBAR_H
