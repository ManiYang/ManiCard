#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>

class QGraphicsView;
class GraphicsScene;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // component widgets:
    QGraphicsView *graphicsView {nullptr};

    //
    GraphicsScene *graphicsScene {nullptr};

    // setup
    void setUpWidgets();
    void installEventFilters();
    QString styleSheet();

    // event handlers
    void onGraphicsViewResize();

};

#endif // BOARDVIEW_H
