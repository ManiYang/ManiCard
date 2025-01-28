#ifndef COLORS_UTIL_H
#define COLORS_UTIL_H

#include <QColor>

inline QColor invertHslLightness(const QColor &color) {
    constexpr double m = 0.4;
    constexpr double a = -4.0 * m + 2.0;
    constexpr double b = 4.0 * m - 3.0;
    constexpr double c = 1.0;
    // y = a x^2 + b x + c is a parabola with y(0) = 1, y(1) = 0, y(0.5) = m

    double h;
    double s;
    double l;
    color.getHslF(&h, &s, &l);

    const double ll = (a * l + b) * l + c;
    return QColor::fromHslF(h, s, ll);
}

inline QColor shiftHslLightness(const QColor &color, const double lightnessShift) {
    double h;
    double s;
    double l;
    color.getHslF(&h, &s, &l);

    l += lightnessShift;
    l = std::max(0.0, l);
    l = std::min(1.0, l);

    return QColor::fromHslF(h, s, l);
}

#endif // COLORS_UTIL_H
