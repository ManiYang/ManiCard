#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>

class QGraphicsView;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

private:
    // component widgets:
    QGraphicsView *graphicsView {nullptr};

    // setup
    void setUpWidgets();
    QString styleSheet();
};

#endif // BOARDVIEW_H
