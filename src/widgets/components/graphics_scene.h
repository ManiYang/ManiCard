#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QBasicTimer>
#include <QGraphicsScene>
#include <QTimer>

//!
//! A subclass of QGraphicsScene that responds to mouse and keyboard events for
//!   - drag-scrolling (scrolling by mouse dragging) the QGraphicsView,
//!   - zooming in/out.
//!
class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GraphicsScene(QObject *parent = nullptr);

signals:
    void dragScrollingEnded();
    void contextMenuRequestedOnScene(const QPointF &scenePos);
    void clickedOnBackground();
    void userToZoomInOut(bool zoomIn, const QPointF &anchorScenePos);

    void viewScrollingStarted();
    void viewScrollingFinished();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    enum class State {
        Normal,
        RightPressed, RightDragScrolling,
        LeftDragScrollStandby, LeftDragScrolling
    };
    State state {State::Normal};

    QPoint mousePressScreenPos {0, 0};
    QPointF viewCenterBeforeDragScroll; // center of view in scene coordinates
    bool isSpaceKeyPressed {false};
    bool isLeftButtonPressed {false};

    QTimer *timerResetAccumulatedWheelDelta;
    int accumulatedWheelDelta {0};

    QTimer *timerFinishViewScrolling;

    void startDragScrolling();
    void dragScroll(
            const QPointF &viewCenterBeforeDragScroll, const QPoint &dragDisplacement);
    void endDragScolling();

    QGraphicsView *getView() const; // can be nullptr
    QPointF getViewCenterInScene() const;

    //!
    //! \return a divisor of 120 (times -1 if \e delta is negative), or 0 \e delta is 0
    //!
    static int discretizeWheelDelta(const int delta);
};

#endif // GRAPHICSSCENE_H
