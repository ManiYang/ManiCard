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
