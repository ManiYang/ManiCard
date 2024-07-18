#ifndef EDGE_ARROW_H
#define EDGE_ARROW_H

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
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
    QColor lineColor {155, 155, 155};
    QString label;

    // child items
    QGraphicsLineItem *lineItem;
    QGraphicsSimpleTextItem *labelItem;

    //
    void adjustChildItems();
};

#endif // EDGE_ARROW_H
