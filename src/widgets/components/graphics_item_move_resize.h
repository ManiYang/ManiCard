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
    //! The resize activation region is the visible part of \e item minus the area given by
    //! item->boundingRect().marginRemoved(borderWidth) .
    //! \param item: must accept hover event and mouse left-button events
    //! \param borderWidth
    //! \param targetItemMinimumSize
    //!
    void setResizeHandle(
            QGraphicsItem *item, const double borderWidth,
            const QSizeF &targetItemMinimumSize);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void getTargetItemPosition(QPointF *pos); // use Qt::DirectConnection
    void setTargetItemPosition(const QPointF &pos);
    void movingEnded();

    void getTargetItemRect(QRectF *rect); // use Qt::DirectConnection
    void setTargetItemRect(const QRectF &rect);
    void resizingEnded();

    void setCursorShape(const std::optional<Qt::CursorShape> cursorShape);

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    QGraphicsItem *moveHandle {nullptr};
    QGraphicsItem *resizeHandle {nullptr};
    double resizeHandleBorderWidth;
    QSizeF targetItemMinSize;

    enum class State {
        Normal, BeforeMove, Moving, Resizing
    };
    State state {State::Normal};

    QPoint mousePressScreenPos;
    QString resizeDirection;
    QPointF targetItemPosBeforeMove;
    QRectF targetItemRectBeforeResize;

    bool eventFilterForMoveHandle(QEvent *event);
    bool eventFilterForResizeHandle(QEvent *event);

    void doResize(const QPoint displacement);

    bool isWithinResizeHandleBorder(const QPointF scenePos) const;
    QString getRegionInResizeHandle(const QPointF scenePos) const;
};

#endif // GRAPHICSITEMMOVERESIZE_H
