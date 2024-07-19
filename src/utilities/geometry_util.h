#ifndef GEOMETRY_UTIL_H
#define GEOMETRY_UTIL_H

#include <QLineF>
#include <QPointF>
#include <QRectF>

//!
//! Find out whether any of the 4 edges of \e rect intersect with \e line. If so,
//! \c *intersectionPoint will be an intersection point.
//!
bool rectEdgeIntersectsWithLine(
        const QRectF &rect, const QLineF &line, QPointF *intersectionPoint);

#endif // GEOMETRY_UTIL_H
