#ifndef GROUPBOXDATA_H
#define GROUPBOXDATA_H

#include <optional>
#include <QJsonObject>
#include <QRectF>
#include <QSet>
#include <QString>

struct GroupBoxDataUpdate;

struct GroupBoxData
{
    // properties of `GroupBox` node
    QString title;
    QRectF rect;

    //
    QSet<int> childGroupBoxes;
    QSet<int> childCards;

    //
    // todo: rename.....
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
