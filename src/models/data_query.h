#ifndef DATAQUERY_H
#define DATAQUERY_H

#include <QJsonObject>
#include <QString>

struct DataQuery
{
    QString title;
    QString queryCypher;
    QJsonObject queryParameters;

    static bool validateCypher(const QString queryCypher, QString *msg = nullptr);
};

#endif // DATAQUERY_H
