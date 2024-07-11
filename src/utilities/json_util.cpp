#include <type_traits>
#include <QFile>
#include <QJsonArray>
#include <QSet>
#include <QString>
#include "json_util.h"
#include "utilities/numbers_util.h"

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

QJsonArray toJsonArray(const QSet<QString> &set) {
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
            *errorMsg = QString("could not open file %1").arg(filePath);
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

JsonReader::JsonReader(const QJsonObject &obj) : currentValue(obj) {
    currentPath.reserve(8);
}

JsonReader::JsonReader(const QJsonArray &arr) : currentValue(arr) {
    currentPath.reserve(8);
}

JsonReader &JsonReader::operator [](const QString &key) {
    currentPath << key;

    if (currentValue.isObject())
        currentValue = currentValue.toObject().value(key);
    else
        currentValue = QJsonValue::Undefined;

    return *this;
}

JsonReader &JsonReader::operator [](const int index) {
    currentPath << index;

    if (currentValue.isArray())
        currentValue = currentValue.toArray().at(index);
    else
        currentValue = QJsonValue::Undefined;

    return *this;
}

QJsonValue JsonReader::get() const {
    return currentValue;
}

QJsonValue JsonReader::getOrThrow() const {
    throwIfCurrentValueUndefined();
    return currentValue;
}

QString JsonReader::getString() const {
    return currentValue.toString();
}

QString JsonReader::getStringOrThrow() const {
    throwIfCurrentValueUndefined();
    if (!currentValue.isString()) {
        const auto errMsg = QString("value at %1 is not a string").arg(getCurrentPathString());
        throw JsonReaderError(errMsg.toStdString());
    }
    return currentValue.toString();
}

int JsonReader::getInt() const {
    return currentValue.toInt();
}

int JsonReader::getIntOrThrow() const {
    throwIfCurrentValueUndefined();
    if (!currentValue.isDouble() || !isInteger(currentValue.toDouble())) {
        const auto errMsg = QString("value at %1 is not an integer").arg(getCurrentPathString());
        throw JsonReaderError(errMsg.toStdString());
    }
    return currentValue.toInt();
}

double JsonReader::getDouble() const {
    return currentValue.toDouble();
}

double JsonReader::getDoubleOrThrow() const {
    throwIfCurrentValueUndefined();
    if (!currentValue.isDouble()) {
        const auto errMsg = QString("value at %1 is not a number").arg(getCurrentPathString());
        throw JsonReaderError(errMsg.toStdString());
    }
    return currentValue.toDouble();
}

bool JsonReader::getBool() const {
    return currentValue.toBool();
}

bool JsonReader::getBoolOrThrow() const {
    throwIfCurrentValueUndefined();
    if (!currentValue.isBool()) {
        const auto errMsg = QString("value at %1 is not a Boolean").arg(getCurrentPathString());
        throw JsonReaderError(errMsg.toStdString());
    }
    return currentValue.toBool();
}

void JsonReader::throwIfCurrentValueUndefined() const {
    if (currentValue.isUndefined()) {
        const auto errMsg = QString("could not find value at %1").arg(getCurrentPathString());
        throw JsonReaderError(errMsg.toStdString());
    }
}

QString JsonReader::getCurrentPathString() const {
    QStringList parts;
    for (const auto &item: currentPath) {
        QString keyOrIndex;
        if (std::holds_alternative<int>(item)) {
            keyOrIndex = QString::number(std::get<int>(item));
        }
        else if (std::holds_alternative<QString>(item)) {
            keyOrIndex = QString("\"%1\"").arg(std::get<QString>(item));
        }
        else {
            Q_ASSERT(false); // not implemented
            keyOrIndex = "?";
        }

        parts << QString("[%1]").arg(keyOrIndex);
    }
    return parts.join("");
}
