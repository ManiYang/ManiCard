#ifndef MAPS_UTIL_H
#define MAPS_UTIL_H

#include <QHash>
#include <QSet>

template <class K, class V>
QSet<K> keySet(const QHash<K, V> &map) {
    QSet<K> result;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        result << it.key();
    return result;
}

template <class K, class V>
QSet<K> keySet(const QMap<K, V> &map) {
    QSet<K> result;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        result << it.key();
    return result;
}

template <class K, class V>
void mergeWith(QHash<K, V> &map1, const QHash<K, V> &map2) {
    for (auto it = map2.constBegin(); it != map2.constEnd(); ++it)
        map1.insert(it.key(), it.value());
}

template <class K, class V>
void mergeWith(QMap<K, V> &map1, const QMap<K, V> &map2) {
    for (auto it = map2.constBegin(); it != map2.constEnd(); ++it)
        map1.insert(it.key(), it.value());
}

#endif // MAPS_UTIL_H
