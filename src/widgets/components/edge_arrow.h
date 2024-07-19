#ifndef EDGE_ARROW_H
#define EDGE_ARROW_H

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QPolygonF>
#include "models/relationship.h"

class EdgeArrow : public QGraphicsItem
{
public:
    explicit EdgeArrow(const RelationshipId &relId_);

    //
    void setStartEndPoint(const QPointF &startPoint, const QPointF &endPoint);
    void setLabel(const QString &label);
    void setLineWidth(const double width);
    void setLineColor(const QColor &color);

    //
    QRectF boundingRect() const override;
    void paint(
            QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    const RelationshipId relId;

    QPointF startPoint;
    QPointF endPoint;
    double lineWidth {2.0};
    QColor lineColor {100, 100, 100};
    QString label;

    // child items
    QGraphicsLineItem *lineItem;
    QGraphicsSimpleTextItem *labelItem;
    QGraphicsPolygonItem *arrowHeadItem;

    //
    void adjustChildItems();

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

    static QPolygonF computeArrowHeadPolygon(const QLineF &line, const double size);
};

#endif // EDGE_ARROW_H
