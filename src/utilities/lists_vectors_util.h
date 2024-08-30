#ifndef LISTS_VECTORS_UTIL_H
#define LISTS_VECTORS_UTIL_H

#include <QVector>

//!
//! \param values
//! \param ordering: may not contain all elements in \e values, and can contain elements not
//!                  included in \e values
//! \param defaultInFront: If true [false], values not found in \e ordering will be placed in the
//!                        front [back] of the resulting vector.
//!
template <class Container1, class Container2>
QVector<typename Container1::value_type> sortByOrdering(
        const Container1 &values, const Container2 &ordering, const bool defaultInFront)
{
    static_assert(std::is_same_v<
            typename Container1::value_type,
            typename Container2::value_type
    >);
    using T = typename Container1::value_type;

    QVector<T> sorted;
    for (const T &v: ordering) {
        if (values.contains(v))
            sorted << v;
    }

    QVector<T> remaining;
    for (const T &v: values) {
        if (!sorted.contains(v))
            remaining << v;
    }

    if (defaultInFront)
        return remaining + sorted;
    else
        return sorted + remaining;
}

#endif // LISTS_VECTORS_UTIL_H
