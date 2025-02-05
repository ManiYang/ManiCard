#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include <QGraphicsScene>
#include "app_data_readonly.h"
#include "edge_arrow.h"
#include "services.h"
#include "utilities/geometry_util.h"
#include "widgets/components/drag_point_events_handler.h"

constexpr double vicinityCriterion = 4;

EdgeArrow::EdgeArrow(QGraphicsItem *parent)
        : QGraphicsObject(parent)
//        , lineItem(new QGraphicsLineItem(this))
        , labelItem(new QGraphicsSimpleTextItem(this))
        , arrowHeadItem(new QGraphicsPolygonItem(this))
        , dragPointEventsHandler(new DragPointEventsHandler(this)) {
    setFlag(QGraphicsItem::ItemHasNoContents);
    setAcceptHoverEvents(true);

    arrowHeadItem->setPen(Qt::NoPen);

    {
        QFont font;
        font.setPixelSize(13);
        labelItem->setFont(font);
    }

    //
    setUpConnections();
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

void EdgeArrow::setLabelColor(const QColor &color) {
    labelItem->setBrush(color);
}

void EdgeArrow::setLineWidth(const double width) {
    lineWidth = width;
    adjustChildItems();
}

void EdgeArrow::setLineColor(const QColor &color) {
    lineColor = color;
    adjustChildItems();
}

void EdgeArrow::setJoints(const QVector<QPointF> &joints_){
    joints = joints_;
    adjustChildItems();
}

QRectF EdgeArrow::boundingRect() const {
    return currentShape.boundingRect();
}

QPainterPath EdgeArrow::shape() const {
    return currentShape;
}

void EdgeArrow::paint(
        QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    // do nothing
}

void EdgeArrow::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    int jointIndex = -1;
    int lineIndex = -1;
    for (int i = 0; i < joints.count(); ++i) {
        if (vicinityOfJoint(joints.at(i)).contains(event->pos())) {
            jointIndex = i;
            break;
        }
    }
    if (jointIndex == -1) {
        for (int i = 0; i < lineItems.count(); ++i) {
            if (vicinityOfLine(lineItems.at(i)->line()).contains(event->pos())) {
                lineIndex = i;
                break;
            }
        }
    }

    //
    dragPointData.clear();

    if (jointIndex != -1) {
        // show circular drag point at the joint
        dragPoint.showAt(joints.at(jointIndex), DragPoint::Shape::Circle);
        dragPointData.atJointIndex = jointIndex;
    }
    else if (lineIndex != - 1) {
        // show square drag point on the line
        constexpr bool limitToLineSegment = true;
        const QPointF p = getProjectionOnLine(
                    event->pos(), lineItems.at(lineIndex)->line(), limitToLineSegment);
        dragPoint.showAt(p, DragPoint::Shape::Square);
        dragPointData.onLineIndex = lineIndex;
    }
    else {
        dragPoint.remove();
    }
}

void EdgeArrow::hoverLeaveEvent(QGraphicsSceneHoverEvent */*event*/) {
    dragPoint.remove();
}

bool EdgeArrow::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
//    if (watched == dragPoint.dragPointItem())
//        return eventFilterForDragPoint(event);

    return QGraphicsItem::sceneEventFilter(watched, event);
}

void EdgeArrow::setUpConnections() {
    connect(dragPointEventsHandler, &DragPointEventsHandler::getPosition,
            this, [this](const int /*itemId*/, QPointF *pos) {
        *pos = dragPoint.center();
    }, Qt::DirectConnection);

    connect(dragPointEventsHandler, &DragPointEventsHandler::positionUpdated,
            this, [this](const int /*itemId*/, const QPointF &pos) {
        // move drag point
        dragPoint.moveTo(pos);

        // move joint
        const int jointIndex = dragPointData.atJointIndex;
        Q_ASSERT(jointIndex >= 0 && jointIndex < joints.count());
        joints[jointIndex] = pos;
        adjustChildItems();
    });

    connect(dragPointEventsHandler, &DragPointEventsHandler::movingStarted,
            this, [this](const int /*itemId*/) {
        if (dragPointData.onLineIndex != -1) {
            // insert a joint
            const int newJointIndex = dragPointData.onLineIndex;
            Q_ASSERT(newJointIndex >= 0 && newJointIndex <= joints.count());
            joints.insert(newJointIndex, dragPoint.center());
            adjustChildItems();

            dragPointData.onLineIndex = -1;
            dragPointData.atJointIndex = newJointIndex;
        }
    });

    connect(dragPointEventsHandler, &DragPointEventsHandler::movingFinished,
            this, [this](const int /*itemId*/) {
        emit jointsUpdated(joints);
    });

    connect(dragPointEventsHandler, &DragPointEventsHandler::doubleClicked,
            this, [this](const int /*itemId*/) {
        if (dragPointData.atJointIndex != -1) {
            // remove the joint
            joints.remove(dragPointData.atJointIndex);
            adjustChildItems();

            dragPointData.atJointIndex = -1;
        }
    });
}

void EdgeArrow::adjustChildItems() {
    // line
//    QLineF line(startPoint, endPoint);
//    lineItem->setLine(line);
//    lineItem->setPen(QPen(QBrush(lineColor), lineWidth));

    // lines
    // -- create/remove line items if necessary
    if (lineItems.count() != joints.count() + 1) {
        while (lineItems.count() < joints.count() + 1)
            lineItems << new QGraphicsLineItem(this);

        while (lineItems.count() > joints.count() + 1) {
            auto *lineItem = lineItems.takeLast();
            if (scene() != nullptr)
                scene()->removeItem(lineItem);
            delete lineItem;
        }
    }
    Q_ASSERT(lineItems.count() == joints.count() + 1);

    // -- update the line items
    for (int i = 0; i < lineItems.count(); ++i) {
        const QPointF p1 = (i == 0) ? startPoint : joints.at(i - 1);
        const QPointF p2 = (i < joints.count()) ? joints.at(i) : endPoint;

        lineItems.at(i)->setLine(QLineF(p1, p2));
        lineItems.at(i)->setPen(QPen(QBrush(lineColor), lineWidth));
    }

    // arrow head
    const double arrowHeadSize = (4.0 * lineWidth * lineWidth + 16) / (lineWidth + 1.0);

//    const QPolygonF polygon = computeArrowHeadPolygon(line, arrowHeadSize);
    const QPolygonF polygon = computeArrowHeadPolygon(
            lineItems.last()->line(), arrowHeadSize);
    arrowHeadItem->setPolygon(polygon);
    arrowHeadItem->setBrush(lineColor);

    // label
    labelItem->setText(label);
    const QSizeF textBoundingSize = labelItem->boundingRect().size();

    constexpr double labelAndLineSpacing = 2.0;
    constexpr bool textIsAbove = true;
//    const auto [textPos, textRotationClockwise] = computeLabelPositionAndRotation(
//            line, textBoundingSize, labelAndLineSpacing, textIsAbove);
    const auto [textPos, textRotationClockwise] = computeLabelPositionAndRotation(
            lineItems.first()->line(), textBoundingSize, labelAndLineSpacing, textIsAbove);

    labelItem->setPos(textPos);
    labelItem->setRotation(textRotationClockwise);

    // update `currentShape`
    currentShape = updateShape();
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

QPainterPath EdgeArrow::updateShape() const {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    for (QGraphicsLineItem *lineItems: qAsConst(lineItems))
        path.addPath(vicinityOfLine(lineItems->line()));

    for (const QPointF &joint: qAsConst(joints))
        path.addPath(vicinityOfJoint(joint));

    return path;
}

QPainterPath EdgeArrow::vicinityOfLine(const QLineF line) {
    return tiltedRect(line, vicinityCriterion * 2.0);
}

QPainterPath EdgeArrow::vicinityOfJoint(const QPointF joint) {
    QPainterPath path;
    path.addRect(squareCenteredAt(joint, vicinityCriterion * 2.0));
    return path;
}

//====

EdgeArrow::DragPoint::DragPoint(EdgeArrow *edgeArrow)
    : edgeArrow(edgeArrow) {
}

void EdgeArrow::DragPoint::showAt(const QPointF &center, const Shape shape) {
    create();

    bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();

    switch (shape) {
    case Shape::Square:
    {
        constexpr double squareSize = vicinityCriterion * 2.0;
        rectItem->setRect(squareCenteredAt(center, squareSize));

        const QColor color = isDarkTheme ? QColor(Qt::white) : QColor(Qt::black);
        rectItem->setBrush(QBrush(color));

        rectItem->setVisible(true);
        circleItem->setVisible(false);
        break;
    }
    case Shape::Circle:
    {
        constexpr double circleSize = vicinityCriterion * 2.83;
        circleItem->setRect(squareCenteredAt(center, circleSize));

        const QColor color = isDarkTheme ? QColor(128, 179, 255) : QColor(0, 71, 179);
        circleItem->setBrush(QBrush(color));

        rectItem->setVisible(false);
        circleItem->setVisible(true);
        break;
    }
    }
}

void EdgeArrow::DragPoint::moveTo(const QPointF &center) {
    if (rectItem != nullptr && rectItem->isVisible()) {
        const double size = rectItem->rect().size().width();
        rectItem->setRect(squareCenteredAt(center, size));
    }
    else if (circleItem != nullptr && circleItem->isVisible()) {
        const double size = circleItem->rect().size().width();
        circleItem->setRect(squareCenteredAt(center, size));
    }
}

void EdgeArrow::DragPoint::remove() {
    if (rectItem != nullptr) {
        edgeArrow->dragPointEventsHandler->removeItem(rectItem);

        if (edgeArrow->scene() != nullptr)
            edgeArrow->scene()->removeItem(rectItem);

        delete rectItem;
        rectItem = nullptr;
    }

    if (circleItem != nullptr) {
        edgeArrow->dragPointEventsHandler->removeItem(circleItem);

        if (edgeArrow->scene() != nullptr)
            edgeArrow->scene()->removeItem(circleItem);

        delete circleItem;
        circleItem = nullptr;
    }
}

QPointF EdgeArrow::DragPoint::center() const {
    if (rectItem != nullptr && rectItem->isVisible())
        return rectItem->rect().center();
    if (circleItem != nullptr && circleItem->isVisible())
        return circleItem->rect().center();
    return {};
}

void EdgeArrow::DragPoint::create() {
    if (rectItem == nullptr) {
        rectItem = new QGraphicsRectItem(edgeArrow);
        rectItem->setPen(Qt::NoPen);
        rectItem->setZValue(edgeArrow->zValue() + 1);

        edgeArrow->dragPointEventsHandler->installOnItem(rectItem, int(Shape::Square));
    }

    if (circleItem == nullptr) {
        circleItem = new QGraphicsEllipseItem(edgeArrow);
        circleItem->setPen(Qt::NoPen);
        circleItem->setZValue(edgeArrow->zValue() + 1);

        edgeArrow->dragPointEventsHandler->installOnItem(circleItem, int(Shape::Circle));
    }
}
