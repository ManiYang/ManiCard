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

//!
//! \param enclosingRect: must enclose \e enclosedRect
//! \param enclosedRect
//! \return a \c QMarginsF \c m so that <tt>enclosingRect == enclosedRect.marginsAdded(m)</tt>.
//!         Returns a null QMarginsF if \e enclosingRect does not enclose \e enclosedRect.
//!
QMarginsF diffMargins(const QRectF &enclosingRect, const QRectF &enclosedRect);

QRectF boundingRectOfRects(const QVector<QRectF> &rects);

#endif // GEOMETRY_UTIL_H
