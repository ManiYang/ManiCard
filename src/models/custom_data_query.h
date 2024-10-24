#ifndef CUSTOMDATAQUERY_H
#define CUSTOMDATAQUERY_H

#include <QJsonObject>
#include <QString>

struct CustomDataQuery
{
    QString title;
    QString queryCypher;
    QJsonObject queryParameters;

    static CustomDataQuery fromJson(const QJsonObject &obj);
    static bool validateCypher(const QString queryCypher, QString *msg = nullptr);
};

#endif // CUSTOMDATAQUERY_H
