#ifndef EDGE_ARROW_H
#define EDGE_ARROW_H

#include <QGraphicsObject>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QPolygonF>

class DragPointEventsHandler;

class EdgeArrow : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit EdgeArrow(QGraphicsItem *parent = nullptr);

    //
    void setStartEndPoint(const QPointF &startPoint, const QPointF &endPoint);
    void setLabel(const QString &label);
    void setLabelColor(const QColor &color);
    void setLineWidth(const double width);
    void setLineColor(const QColor &color);
    void setJoints(const QVector<QPointF> &joints);

    //
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void jointsUpdated(const QVector<QPointF> &joints);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

private:
    QPointF startPoint;
    QPointF endPoint;
    double lineWidth {2.0};
    QColor lineColor {100, 100, 100};
    QString label;
    QVector<QPointF> joints;

//    enum class State { Normal, DraggingJoint };
//    State state {State::Normal};
    QPainterPath currentShape;

    // child items
//    QGraphicsLineItem *lineItem;
    QVector<QGraphicsLineItem *> lineItems;
    QGraphicsSimpleTextItem *labelItem;
    QGraphicsPolygonItem *arrowHeadItem;

    // drag point
    struct DragPoint
    {
        explicit DragPoint(EdgeArrow *edgeArrow);
        enum class Shape {Square, Circle};
        void showAt(const QPointF &center, const Shape shape); // also sets the color
        void moveTo(const QPointF &center);
        void remove();
        //
//        QGraphicsItem *dragPointItem() const; // can be nullptr
        QPointF center() const;
    private:
        EdgeArrow *const edgeArrow;
        QGraphicsRectItem *rectItem {nullptr};
        QGraphicsEllipseItem *circleItem {nullptr};
        void create();
    };
    DragPoint dragPoint {this};

    struct DragPointData
    {
        int onLineIndex {-1};
        int atJointIndex {-1};

        void clear() {
            onLineIndex = -1;
            atJointIndex = -1;
        }
    };
    DragPointData dragPointData;

    DragPointEventsHandler *dragPointEventsHandler;

    //
    void setUpConnections();

    void adjustChildItems();

    bool eventFilterForDragPoint(QEvent *event);

    //

    //!
    //! \param line
    //! \param labelBoundingSize
    //! \param spacing: between label text and line
    //! \param textIsAbove: if true [false], put text above [below] the line
    //! \return (position, rotation-angle-clockwise)
    //!
    static std::pair<QPointF, double> computeLabelPositionAndRotation(
            const QLineF &line, const QSizeF &labelBoundingSize,
            const double spacing, const bool textIsAbove);

    //!
    //! \param line
    //! \param size: approximate length of the arrow head
    //! \return
    //!
    static QPolygonF computeArrowHeadPolygon(const QLineF &line, const double size);

    QPainterPath updateShape() const;
    static QPainterPath vicinityOfLine(const QLineF line);
    static QPainterPath vicinityOfJoint(const QPointF joint);
};

#endif // EDGE_ARROW_H
