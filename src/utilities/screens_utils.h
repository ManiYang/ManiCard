#ifndef SCREENS_UTILS_H
#define SCREENS_UTILS_H

#include <QScreen>

//!
//! \return \c nullptr if not found
//!
QScreen *getScreenContaining(const QPoint &point);

//!
//! \return the screen that intersects with the largest part of \e rect.
//!         Returns \c nullptr if no screen intersects with `rect`.
//!
QScreen *getScreenIntersectingMostPart(const QRect &rect);

#endif // SCREENS_UTILS_H
