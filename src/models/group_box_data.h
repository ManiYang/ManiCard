#ifndef GROUPBOXDATA_H
#define GROUPBOXDATA_H

#include <optional>
#include <QJsonObject>
#include <QRectF>
#include <QSet>
#include <QString>

struct GroupBoxNodePropertiesUpdate;

struct GroupBoxData
{
    // properties of `GroupBox` node
    QString title;
    QRectF rect;

    //
    QSet<int> childGroupBoxes;
    QSet<int> childCards; // child NodeRect's corresponding card ID's

    //
    QJsonObject getNodePropertiesJson() const;

    bool updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const GroupBoxNodePropertiesUpdate &update);
};

struct GroupBoxNodePropertiesUpdate
{
    std::optional<QString> title;
    std::optional<QRectF> rect;

    QJsonObject toJson() const;
};

#endif // GROUPBOXDATA_H
