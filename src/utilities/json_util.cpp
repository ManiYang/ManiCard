#include <QFile>
#include <QJsonArray>
#include "json_util.h"

QJsonArray toJsonArray(const QStringList &list) {
    QJsonArray array;
    for (const auto &item: list)
        array << item;
    return array;
}

QJsonArray toJsonArray(const QSet<int> &set) {
    QJsonArray array;
    for (const auto &item: set)
        array << item;
    return array;
}

QStringList toStringList(const QJsonArray &array, const QString &defaultValue) {
    QStringList result;
    for (const QJsonValue &v: array)
        result << v.toString(defaultValue);
    return result;
}

QJsonDocument readJsonFile(const QString &filePath, QString *errorMsg) {
    QFile file(filePath);
    const bool ok = file.open(QIODevice::ReadOnly);
    if (!ok) {
        if (errorMsg)
            *errorMsg = "could not open file";
        return {};
    }

    QJsonParseError jsonParseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &jsonParseError);
    if (errorMsg)
        *errorMsg = jsonParseError.errorString();
    return doc;
}

QJsonObject parseAsJsonObject(const QString &json, QString *errorMsg) {
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        if (errorMsg)
            *errorMsg = err.errorString();
        return {};
    }
    if (!doc.isObject()) {
        if (errorMsg)
            *errorMsg = "the JSON document is not an object";
        return {};
    }
    return doc.object();
}

QJsonArray parseAsJsonArray(const QString &json, QString *errorMsg) {
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        if (errorMsg)
            *errorMsg = err.errorString();
        return {};
    }
    if (!doc.isArray()) {
        if (errorMsg)
            *errorMsg = "the JSON document is not an array";
        return {};
    }
    return doc.array();
}

QJsonValue getNestedValue(const QJsonObject &object, const QStringList &pathOfKeys) {
    QJsonValue v = object;
    for (const QString &key: pathOfKeys) {
        v = v[key];
        if (v.isUndefined())
            break;
    }
    return v;
}

//====

JsonReader &JsonReader::operator [](const QString &key) {
    if (currentValue.isObject())
        currentValue = currentValue.toObject().value(key);
    else
        currentValue = QJsonValue::Undefined;

    return *this;
}

JsonReader &JsonReader::operator [](const int index) {
    if (currentValue.isArray())
        currentValue = currentValue.toArray().at(index);
    else
        currentValue = QJsonValue::Undefined;

    return *this;
}

QJsonValue JsonReader::get() const {
    return currentValue;
}

