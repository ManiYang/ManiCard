#ifndef STRINGS_UTIL_H
#define STRINGS_UTIL_H

#include <QSet>
#include <QString>

QString joinStringSet(const QSet<QString> &set, const QString &sep, const bool sort = false);

#endif // STRINGS_UTIL_H
