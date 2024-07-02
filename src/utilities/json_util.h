#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

inline QString printJson(const QJsonObject &object, const bool compact = true) {
    return QJsonDocument(object).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

inline QString printJson(const QJsonArray &array, const bool compact = true) {
    return QJsonDocument(array).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

//!
//! \return a null document if failed
//!
QJsonDocument readJsonFile(const QString &filePath, QString *errorMsg = nullptr);

//!
//! For example, if \e pathOfKeys is {"a", "b", "c"}, this method returns \c object["a"]["b"]["c"].
//! Returns \c QJsonValue::Undefined if the key-path is not found.
//! Returns \e object itself if \a pathOfKeys is empty.
//!
QJsonValue getNestedValue(const QJsonObject &object, const QStringList &pathOfKeys);

class JsonReader
{
public:
    JsonReader(const QJsonObject &obj) : currentValue(obj) {}
    JsonReader(const QJsonArray &arr) : currentValue(arr) {}

    JsonReader &operator [](const QString &key);
    JsonReader &operator [](const int index);

    //
    QJsonValue get() const;

    QString getString() const {
        return get().toString();
    }

    int getInt() const {
        return get().toInt();
    }

    bool getBool() const {
        return get().toBool();
    }

private:
    QJsonValue currentValue;
};



#endif // JSON_UTIL_H
