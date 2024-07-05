#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stdexcept>
#include <variant>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QStringList>

QJsonArray toJsonArray(const QStringList &list);
QJsonArray toJsonArray(const QSet<int> &set);

QStringList toStringList(const QJsonArray &array, const QString &defaultValue);

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
    JsonReader(const QJsonObject &obj);
    JsonReader(const QJsonArray &arr);

    JsonReader &operator [](const QString &key); // "zoom" into the value at the key
    JsonReader &operator [](const int index); // "zoom" into the value at the index

    //!
    //! The returned value can be \c QJsonValue::Undefined .
    //!
    QJsonValue get() const;

    //!
    //! Throws \c JsonReaderException if current value is \c QJsonValue::Undefined .
    //!
    QJsonValue getOrThrow() const;

    QString getString() const;
    QString getStringOrThrow() const; // throws \c JsonReaderException
    int getInt() const;
    int getIntOrThrow() const; // throws \c JsonReaderException
    double getDouble() const;
    double getDoubleOrThrow() const; // throws \c JsonReaderException
    bool getBool() const;
    bool getBoolOrThrow() const; // throws \c JsonReaderException

private:
    QJsonValue currentValue;

    using IntOrString = std::variant<int, QString>;
    QVector<IntOrString> currentPath;

    void throwIfCurrentValueUndefined() const;

    //!
    //! For example, if \c currentPath is {"a", 1}, returns "[\"a\"][1]"
    //!
    QString getCurrentPathString() const;
};

class JsonReaderException : public std::runtime_error
{
public:
    explicit JsonReaderException(const std::string &whatArg)
        : std::runtime_error(whatArg) {}
};

#endif // JSON_UTIL_H
