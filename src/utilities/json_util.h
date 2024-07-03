#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

//!
//! \return a null document if failed
//!
QJsonDocument readJsonFile(const QString &filePath, QString *errorMsg = nullptr);

QJsonObject parseAsJsonObject(const QString &json, QString *errorMsg = nullptr);
QJsonArray parseAsJsonArray(const QString &json, QString *errorMsg = nullptr);

inline QString printJson(const QJsonObject &object, const bool compact = true) {
    return QJsonDocument(object).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}
inline QString printJson(const QJsonArray &array, const bool compact = true) {
    return QJsonDocument(array).toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

//!
//! For example, if \e pathOfKeys is {"a", "b", "c"}, this method returns \c object["a"]["b"]["c"].
//! Returns \c QJsonValue::Undefined if the key-path is not found.
//! Returns \e object itself if \a pathOfKeys is empty.
//!
QJsonValue getNestedValue(const QJsonObject &object, const QStringList &pathOfKeys);

//!
//! Usage examples:
//!   Suppose \c obj is
//!   \code{.json}
//!     {
//!       "a": {
//!         "b": [1, {"c": 2}]
//!       }
//!     }
//!   \endcode
//!   Then
//!   \code
//!     JsonReader(obj)["a"]["b"][0].getInt(); // 1
//!     JsonReader(obj)["a"]["b"][1].get(); // object {"c": 2}
//!     JsonReader(obj)["a"]["b"][1]["c"].getInt(); // 2
//!     JsonReader(obj)["x"]["y"].get(); // QJsonValue::Undefined
//!     JsonReader(obj)[0].get(); // QJsonValue::Undefined
//!   \endcode
//!
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
