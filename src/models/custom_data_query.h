#ifndef CUSTOMDATAQUERY_H
#define CUSTOMDATAQUERY_H

#include <optional>
#include <QJsonObject>
#include <QString>

struct CustomDataQueryUpdate;

struct CustomDataQuery
{
    QString title;
    QString queryCypher;
    QJsonObject queryParameters;

    //
    void update(const CustomDataQueryUpdate &update);

    //!
    //! The value of \c queryParameters is converted to string.
    //!
    QJsonObject toJson() const;

    //!
    //! \param obj: the value of key "queryParameters", if exists, must be of string type
    //!
    static CustomDataQuery fromJson(const QJsonObject &obj);
    static bool validateCypher(const QString queryCypher, QString *msg = nullptr);
};


struct CustomDataQueryUpdate
{
    std::optional<QString> title;
    std::optional<QString> queryCypher;
    std::optional<QJsonObject> queryParameters;

    //
    void mergeWith(const CustomDataQueryUpdate &other);

    //!
    //! The wrapped value of \c queryParameters, if exists, is converted to string.
    //!
    QJsonObject toJson() const;
};

#endif // CUSTOMDATAQUERY_H
