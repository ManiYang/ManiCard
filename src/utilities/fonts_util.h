#ifndef FONTS_UTIL_H
#define FONTS_UTIL_H

#include <QRectF>
#include <QSet>
#include <QWidget>

QRectF boundingRectOfString(
        const QString &str, const QString &fontFamily, const int pointSize,
        const bool bold = false, const bool italic = false, const QWidget *widget = nullptr);

//!
//! On Windows, user can set the font-size scale factor for a high-DPI monitor. This function can
//! be used to obtain an approximate value of the scale factor.
//! \return an approximate value of the font-size scale factor of the display on which `widget`
//!         is shown
//!
double fontSizeScaleFactor(const QWidget *widget);

QStringList getFontFamilies();

#endif // FONTS_UTIL_H
