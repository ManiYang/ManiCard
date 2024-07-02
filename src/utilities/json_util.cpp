#include <QFile>
#include <QJsonArray>
#include "json_util.h"

QJsonDocument readJsonFile(const QString &filePath, QString *errorMsg)
{
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

QJsonValue JsonReader::get() const {
    return currentValue;
}

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
