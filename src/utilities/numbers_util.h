#ifndef NUMBERS_UTIL_H
#define NUMBERS_UTIL_H

#include <cmath>

inline int nearestInteger(const double x) {
    return static_cast<int>(std::floor(x + 0.5));
}

inline bool isInteger(const double x, const double tolerance = 1e-8) {
    return std::fabs(x - nearestInteger(x)) <= tolerance;
}

#endif // NUMBERS_UTIL_H
