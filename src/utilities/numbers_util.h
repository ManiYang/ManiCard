#ifndef NUMBERS_UTIL_H
#define NUMBERS_UTIL_H

#include <cmath>
#include <QPointF>
#include <QSizeF>

inline int nearestInteger(const double x) {
    return static_cast<int>(std::floor(x + 0.5));
}

inline int ceilInteger(const double x) {
    return static_cast<int>(std::ceil(x));
}

inline bool isInteger(const double x, const double tolerance = 1e-8) {
    return std::fabs(x - nearestInteger(x)) <= tolerance;
}

inline double vectorLength(const QPoint v) {
    return std::sqrt(double(v.x()) * v.x() + double(v.y()) * v.y());
}

inline double vectorLength(const QPointF v) {
    return std::sqrt(v.x() * v.x() + v.y() * v.y());
}

inline QPointF quantize(const QPointF &p, const double step) {
    return {
        std::floor(p.x() / step + 0.5) * step,
        std::floor(p.y() / step + 0.5) * step
    };
}

inline QSizeF quantize(const QSizeF &s, const double step) {
    return {
        std::floor(s.width() / step + 0.5) * step,
        std::floor(s.height() / step + 0.5) * step
    };
}

#endif // NUMBERS_UTIL_H
