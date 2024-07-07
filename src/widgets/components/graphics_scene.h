#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QGraphicsScene>

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GraphicsScene(QObject *parent = nullptr);

signals:
    void dragScrollingEnded();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    enum class State {
        Normal,
        RightPressed, RightDragScrolling,
        LeftDragScrollStandby, LeftDragScrolling
    };
    State mState {State::Normal};

    QPoint mousePressScreenPos {0, 0};
    QPointF viewCenterBeforeDragScroll; // center of view in scene coordinates
    bool isSpaceKeyPressed {false};
    bool isLeftButtonPressed {false};

    void startDragScrolling();
    void onDragScrolled(
            const QPointF &viewCenterBeforeDragScroll, const QPoint &dragDisplacement);
    void endDragScolling();

    QGraphicsView *getView() const; // can be nullptr
    QPointF getViewCenterInScene() const;
};

#endif // GRAPHICSSCENE_H
