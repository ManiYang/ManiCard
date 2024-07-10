#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>

class GraphicsScene;
class QGraphicsRectItem;
class QGraphicsView;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    // component widgets:
    QGraphicsView *graphicsView {nullptr};

    //
    GraphicsScene *graphicsScene {nullptr};

    //
    bool isEverShown {false};

    // setup
    void setUpWidgets();
    void installEventFiltersOnComponents();
    QString styleSheet();

    // event handlers
    void onShownForFirstTime();
    void onGraphicsViewResize();



};

#endif // BOARDVIEW_H
