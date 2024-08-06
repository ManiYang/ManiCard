#ifndef MARGINS_UTIL_H
#define MARGINS_UTIL_H

#include <QMargins>
#include <QMarginsF>

inline QMarginsF uniformMarginsF(const double margin) {
    return {margin, margin, margin, margin};
}

inline QMargins uniformMargins(const int margin) {
    return {margin, margin, margin, margin};
}

#endif // MARGINS_UTIL_H
