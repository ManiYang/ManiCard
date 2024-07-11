#ifndef MARGINS_UTIL_H
#define MARGINS_UTIL_H

#include <QMarginsF>

inline QMarginsF uniformMargins(const double margin) {
    return {margin, margin, margin, margin};
}

#endif // MARGINS_UTIL_H
