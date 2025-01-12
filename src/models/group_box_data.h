#ifndef GROUPBOXDATA_H
#define GROUPBOXDATA_H

#include <optional>
#include <QJsonObject>
#include <QRectF>
#include <QString>

struct GroupBoxDataUpdate;

struct GroupBoxData
{
    QString title;
    QRectF rect;

    QJsonObject toJson() const;
    static std::optional<GroupBoxData> fromJson(const QJsonObject &obj);
    void update(const GroupBoxDataUpdate &update);
};

struct GroupBoxDataUpdate
{
    std::optional<QString> title;
    std::optional<QRectF> rect;

    QJsonObject toJson() const;
};

#endif // GROUPBOXDATA_H
