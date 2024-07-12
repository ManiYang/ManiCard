#include <algorithm>
#include <QStringList>
#include "strings_util.h"

QString joinStringSet(const QSet<QString> &set, const QString &sep, const bool sort) {
    QStringList list(set.constBegin(), set.constEnd());
    if (sort)
        std::sort(list.begin(), list.end());
    return list.join(sep);
}
