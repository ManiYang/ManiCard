#include <QPen>
#include "edge_arrow.h"

EdgeArrow::EdgeArrow(const RelationshipId &relId_)
        : relId(relId_)
        , lineItem(new QGraphicsLineItem(this))
        , labelItem(new QGraphicsSimpleTextItem(this)) {
    setFlag(QGraphicsItem::ItemHasNoContents);
}

void EdgeArrow::setStartEndPoint(const QPointF &startPoint_, const QPointF &endPoint_) {
    startPoint = startPoint_;
    endPoint = endPoint_;
    adjustChildItems();
}

void EdgeArrow::setLabel(const QString &label_) {
    label = label_;
    adjustChildItems();
}

void EdgeArrow::setLineWidth(const double width) {
    lineWidth = width;
    adjustChildItems();
}

void EdgeArrow::setLineColor(const QColor &color) {
    lineColor = color;
    adjustChildItems();
}

QRectF EdgeArrow::boundingRect() const {
    return {};
}

void EdgeArrow::paint(
        QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    // do nothing
}

void EdgeArrow::adjustChildItems() {
    lineItem->setLine(QLineF(startPoint, endPoint));
    lineItem->setPen(QPen(QBrush(lineColor), lineWidth));

    // arrow head...

    // label ...

}
