#include <QApplication>
#include "screens_utils.h"

QScreen *getScreenContaining(const QPoint &point) {
    const auto allScreens = qApp->screens();
    for (QScreen *screen: allScreens) {
        if (screen->availableGeometry().contains(point))
            return screen;
    }
    return nullptr;
}

QScreen *getScreenIntersectingMostPart(const QRect &rect) {
    const auto allScreens = qApp->screens();

    int maxIntersectionArea = 0;
    QScreen *result = nullptr;
    for (QScreen *screen: allScreens) {
        const auto intersectionRect = screen->availableGeometry().intersected(rect);
        if (!intersectionRect.isNull()) {
            const int area = intersectionRect.width() * intersectionRect.height();
            if (area > maxIntersectionArea) {
                maxIntersectionArea = area;
                result = screen;
            }
        }
    }

    return result;
}

