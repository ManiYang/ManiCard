#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <QJsonDocument>
#include <QJsonObject>

inline QString printJson(const QJsonObject &object, const bool compact = true) {
    return QJsonDocument(object).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

inline QString printJson(const QJsonArray &array, const bool compact = true) {
    return QJsonDocument(array).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

#endif // JSON_UTIL_H
