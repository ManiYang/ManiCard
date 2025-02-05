#ifndef GEOMETRY_UTIL_H
#define GEOMETRY_UTIL_H

#include <QLineF>
#include <QPainterPath>
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

QRectF rectCenteredAt(const QPointF &center, const QSizeF &size);
QRectF squareCenteredAt(const QPointF &center, const double &size);

//!
//! \return the shape resulting from adding \e lineThickness to \e line
//!
QPainterPath tiltedRect(const QLineF &line, const double lineThickness);

//!
//! \param point
//! \param line
//! \param limitToLineSegment: If true, the returned point is on the line segment defined by \e
//!             line. Otherwise, the returned point can be on an extended part of the line segment.
//!
QPointF getProjectionOnLine(
        const QPointF &point, const QLineF &line, const bool limitToLineSegment);


#endif // GEOMETRY_UTIL_H
