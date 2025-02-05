#ifndef DRAG_POINT_EVENT_HANDLER_H
#define DRAG_POINT_EVENT_HANDLER_H

#include <QGraphicsObject>
#include <QHash>

class DragPointEventsHandler : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit DragPointEventsHandler(QGraphicsItem *parent = nullptr);

    //!
    //! You can install this handler on multiple items. Normally at most one of them can become the
    //! mouse grabber when mouse is pressed.
    //!
    void installOnItem(QGraphicsItem *item, const int itemId);

    //!
    //! Stop handling events for \e item.
    //!
    void removeItem(QGraphicsItem *item);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void getPosition(const int itemId, QPointF *pos); // use direct connection
    void positionUpdated(const int itemId, const QPointF &pos);
    void movingStarted(const int itemId);
    void movingFinished(const int itemId);
    void doubleClicked(const int itemId);

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    QHash<QGraphicsItem *, int> watchedItems; // values are item ID's

    enum class State { Normal, Pressed, Moving };
    enum class Event { MousePress, MouseMove, MouseRelease, MouseDoubleClick };
    struct EventParameters
    {
        QPointF mousePos;
        int itemId;
    };

    State state {State::Normal};
    QPointF mousePressPos;
    QPointF itemPosAtMousePress;
    std::optional<int> activeItemId;

    void handleEvent(const Event event, const EventParameters &parameters);
};

#endif // DRAG_POINT_EVENT_HANDLER_H
