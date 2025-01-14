#ifndef GRAPHICSITEMMOVERESIZE_H
#define GRAPHICSITEMMOVERESIZE_H

#include <QGraphicsObject>
#include <QSet>

class GraphicsItemMoveResize : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit GraphicsItemMoveResize(QGraphicsItem *parent = nullptr);

    //!
    //! \param item: must accept mouse left-button events
    //!
    void setMoveHandle(QGraphicsItem *item);

    //!
    //! The resize activation region is the visible part of \e item, subtracted by the area
    //! given by \c item->boundingRect().marginRemoved(maxWidth) .
    //! \param item: must accept hover event and mouse left-button events
    //! \param maxWidth: in pixel
    //! \param targetItemMinimumSize: in pixel
    //!
    void setResizeHandle(
            QGraphicsItem *item, const double maxWidth,
            const QSizeF &targetItemMinimumSize);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void aboutToMove();

    void getTargetItemPosition(QPointF *pos); // use Qt::DirectConnection
    void setTargetItemPosition(const QPointF &pos);
            // the `pos` parameter in the above 2 methods must be in the same coordinates
            // (not necessarily the scene or screen coordinates) whose length unit is pixel.
    void movingEnded();

    void getTargetItemRect(QRectF *rect); // use Qt::DirectConnection
    void setTargetItemRect(const QRectF &rect);
            // the `rect` parameter in the above 2 methods must be in the same coordinates
            // (not necessarily the scene or screen coordinates) whose length unit is pixel.
    void resizingEnded();

    void setCursorShape(const std::optional<Qt::CursorShape> cursorShape);

    void leftMousePressedOnResizeActivationArea();
            // The resize handle itself won't receive the `QGraphicsSceneMouseEvent` event when
            // mouse left button pressed on it within the resize activation area. If the resize
            // handle needs to know when this happends, it can connect to this signal.

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    QGraphicsItem *moveHandle {nullptr};
    QGraphicsItem *resizeHandle {nullptr};
    double resizeHandleBorderWidth; // (pix)
    QSizeF targetItemMinSize; // (pix)

    enum class State {
        Normal, BeforeMove, Moving, Resizing
    };
    State state {State::Normal};

    QPoint mousePressScreenPos;
    QString resizeDirection;
    QPointF targetItemPosBeforeMove; // (pix)
    QRectF targetItemRectBeforeResize; // (pix)

    bool eventFilterForMoveHandle(QEvent *event);
    bool eventFilterForResizeHandle(QEvent *event);

    void doResize(const QPoint displacement); // `displacement`: in pix

    bool isWithinResizeHandleBorder(const QPointF scenePos) const;
    QString getRegionInResizeHandle(const QPointF scenePos) const;
};

#endif // GRAPHICSITEMMOVERESIZE_H
