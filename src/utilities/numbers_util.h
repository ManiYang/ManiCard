#ifndef NUMBERS_UTIL_H
#define NUMBERS_UTIL_H

#include <cmath>
#include <QPointF>

inline int nearestInteger(const double x) {
    return static_cast<int>(std::floor(x + 0.5));
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

#endif // NUMBERS_UTIL_H
