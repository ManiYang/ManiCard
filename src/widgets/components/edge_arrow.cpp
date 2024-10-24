#include <QFont>
#include <QPen>
#include "edge_arrow.h"

EdgeArrow::EdgeArrow(const RelationshipId &relId_, QGraphicsItem *parent)
        : QGraphicsItem(parent)
        , relId(relId_)
        , lineItem(new QGraphicsLineItem(this))
        , labelItem(new QGraphicsSimpleTextItem(this))
        , arrowHeadItem(new QGraphicsPolygonItem(this)) {
    setFlag(QGraphicsItem::ItemHasNoContents);

    arrowHeadItem->setPen(Qt::NoPen);

    {
        QFont font;
        font.setPixelSize(14);
        labelItem->setFont(font);
    }
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
    // line
    QLineF line(QLineF(startPoint, endPoint));
    lineItem->setLine(line);
    lineItem->setPen(QPen(QBrush(lineColor), lineWidth));

    // arrow head
    const QPolygonF polygon = computeArrowHeadPolygon(line, 12);
    arrowHeadItem->setPolygon(polygon);
    arrowHeadItem->setBrush(lineColor);

    // label
    labelItem->setText(label);
    const QSizeF textBoundingSize = labelItem->boundingRect().size();

    constexpr double labelAndLineSpacing = 2.0;
    constexpr bool textIsAbove = true;
    const auto [textPos, textRotationClockwise] = computeLabelPositionAndRotation(
            line, textBoundingSize, labelAndLineSpacing, textIsAbove);

    labelItem->setPos(textPos);
    labelItem->setRotation(textRotationClockwise);
}

std::pair<QPointF, double> EdgeArrow::computeLabelPositionAndRotation(
        const QLineF &line, const QSizeF &labelBoundingSize,
        const double spacing, const bool textIsAbove) {
    const double lineAngle = line.angle();
    const bool lineIsTowardLeft = (90 <= lineAngle && lineAngle < 270);

    // find pointA := intersection of `line` with text's left edge
    QLineF line1 = lineIsTowardLeft
            ? QLineF(line.center(), line.p2())
            : QLineF(line.center(), line.p1());
    line1.setLength(labelBoundingSize.width() / 2.0);
    const QPointF pointA = line1.p2();

    // find (top-left) position of text
    QLineF line2 {pointA, pointA + QPointF {-1, 0}};
    const bool clockwise = (lineIsTowardLeft == textIsAbove);
    line2.setAngle(clockwise ? (lineAngle - 90.0) : (lineAngle + 90.0));
    line2.setLength(textIsAbove ? (spacing + labelBoundingSize.height()) : spacing);
    const QPointF textPos = line2.p2();

    // find rotation of text
    const double textRotationClockwise = lineIsTowardLeft ? (180.0 - lineAngle) : (-lineAngle);

    //
    return {textPos, textRotationClockwise};
}

QPolygonF EdgeArrow::computeArrowHeadPolygon(const QLineF &line, const double size) {
    constexpr double theta = 27; // degree

    const QLineF unitLine = QLineF(line.p2(), line.p1()).unitVector();
    const double angle0 = unitLine.angle();

    QLineF line1 = unitLine;
    line1.setAngle(angle0 + theta);
    line1.setLength(size);

    QLineF line2 = unitLine;
    line2.setAngle(angle0 - theta);
    line2.setLength(size);

    return QPolygonF(QVector<QPointF> {line1.p2(), line1.p1(), line2.p2(), line1.p2()});
}
