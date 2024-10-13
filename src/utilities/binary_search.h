#ifndef BINARY_SEARCH_H
#define BINARY_SEARCH_H

#include <QVector>

//!
//! Type \e T must have comparison operators < and <= implemented.
//! \param values: non-empty & strictly inscreasing
//! \param x:
//! \param intervalClosedLeft:
//! \return -1,              if x is outside and to the left of all the intervals;
//!         values.size()-1, if x is outside and to the right of all the intervals;
//!         p,               if x is within the [values[p], values[p+1]) or
//!                          (values[p], values[p+1]] (depending on \e intervalClosedLeft).
template <class T>
int binarySearchInterval(const QVector<T> &values, const T &x, bool intervalClosedLeft) {
    Q_ASSERT(!values.isEmpty());

    if (intervalClosedLeft) {
        if (x < values.first())
            return -1;
        if (values.last() <= x)
            return values.size() - 1;
    }
    else {
        if (x <= values.first())
            return -1;
        if (values.last() < x)
            return values.size() - 1;
    }

    if (values.size() == 2)
        return 0;

    int p0 = 0;
    int p1 = values.size() - 1;
    int m = (p0 + p1) / 2;
    for (/*nothing*/; p1 - p0 > 1; m = (p0 + p1) / 2) {
        if (intervalClosedLeft) {
            if (x < values.at(m))
                p1 = m;
            else
                p0 = m;
        }
        else {
            if (x <= values.at(m))
                p1 = m;
            else
                p0 = m;
        }
    }
    return p0;
}

//!
//! The \c value_type of \e Container must be convertible to \c double.
//! \param values: must be non-empty & strictly increasing
//! \param x
//! \param preferLeft
//! \return p so that, among the elements in values[], values[p] is the one closest to \e x
//!
template <class Container>
int findIndexOfClosestValue(const Container &values, const double x, bool preferLeft) {
    Q_ASSERT(!values.isEmpty());
    if (values.size() == 1)
        return values.at(0);

    QVector<double> midPoints(values.size() - 1);
    for (int i = 0; i < midPoints.size(); ++i)
        midPoints[i] = (values.at(i) + values.at(i + 1)) / 2.0;

    int p = binarySearchInterval(midPoints, x, !preferLeft);
    Q_ASSERT(p >= -1 && p <=values.size() - 2);
    return p + 1;
}

#endif // BINARY_SEARCH_H
