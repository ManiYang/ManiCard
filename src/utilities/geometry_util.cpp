#include <QVector>
#include "geometry_util.h"

bool rectEdgeIntersectsWithLine(
        const QRectF &rect, const QLineF &line, QPointF *intersectionPoint) {
    const QVector<QLineF> edges {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())
    };
    for (const QLineF &edge: edges) {
        const auto intersectType = edge.intersects(line, intersectionPoint);
        if (intersectType == QLineF::BoundedIntersection) {
            return true;
        }
    }
    return false;
}

QMarginsF diffMargins(const QRectF &enclosingRect, const QRectF &enclosedRect) {
    if (!enclosingRect.contains(enclosedRect))
        return QMarginsF {};
    return QMarginsF {
        enclosedRect.left() - enclosingRect.left(),
        enclosedRect.top() - enclosingRect.top(),
        enclosingRect.right() - enclosedRect.right(),
        enclosingRect.bottom() - enclosedRect.bottom()
    };
}

QRectF boundingRectOfRects(const QVector<QRectF> &rects) {
    QRectF result;
    for (const QRectF &rect: rects) {
        if (rect.isNull())
            continue;

        if (result.isNull())
            result = rect;
        else
            result = result.united(rect);
    }
    return result;
}

QRectF rectCenteredAt(const QPointF &center, const QSizeF &size) {
    return QRectF(
            center - QPointF(size.width() / 2.0, size.height() / 2.0),
            size);
}

QRectF squareCenteredAt(const QPointF &center, const double &size) {
    return QRectF(
            center - QPointF(size / 2.0, size / 2.0),
            QSizeF(size, size));
}

QPainterPath tiltedRect(const QLineF &line, const double lineThickness) {
    if (line.p1() == line.p2())
        return {};

    const double w = std::max(lineThickness, 0.1);

    const QLineF normalVector = line.normalVector().unitVector();
    const QPointF vecN = (normalVector.p2() - normalVector.p1()) * (w / 2.0);

    QPainterPath path;
    path.moveTo(line.p1() + vecN);
    path.lineTo(line.p2() + vecN);
    path.lineTo(line.p2() - vecN);
    path.lineTo(line.p1() - vecN);
    path.closeSubpath();

    return path;
}

QPointF getProjectionOnLine(
        const QPointF &point, const QLineF &line, const bool limitToLineSegment) {
    const double lineLength = line.length();
    if (lineLength < 1e-4)
        return line.p1();

    //
    const QLineF vecP1ToP(line.p1(), point);
    if (vecP1ToP.length() < 1e-4)
        return line.p1();

    //
    const double innerProduct = line.dx() * vecP1ToP.dx() + line.dy() * vecP1ToP.dy();
    double t = innerProduct / lineLength / lineLength;
    if (limitToLineSegment) {
        if (t < 0.0)
            t = 0.0;
        else if (t > 1.0)
            t = 1.0;
    }
    return line.pointAt(t);
}
