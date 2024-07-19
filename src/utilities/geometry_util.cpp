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
