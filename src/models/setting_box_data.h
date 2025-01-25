#ifndef SETTINGBOXDATA_H
#define SETTINGBOXDATA_H

#include <QHash>
#include <QJsonObject>
#include <QRectF>
#include "models/settings/settings.h"

struct SettingBoxDataUpdate;

struct SettingBoxData
{
    SettingTargetType targetType;
    SettingCategory category;
    QRectF rect;

    //
    QString getTargetTypeId() const;
    QString getCategoryId() const;

    //
    QJsonObject toJson() const;
    static std::optional<SettingBoxData> fromJson(const QJsonObject &obj);
    void update(const SettingBoxDataUpdate &update);

    // target-type ID's and category ID's to be stored in DB
    static QString getSettingTargetTypeIdForDb(const SettingTargetType targetType);
    static QString getSettingCategoryIdForDb(const SettingCategory category);
    static std::optional<SettingTargetType> getSettingTargetTypeFromIdForDb(const QString &id);
    static std::optional<SettingCategory> getSettingCategoryFromIdForDb(const QString &id);

    static const QHash<SettingTargetType, QString> settingTargetTypeToIdForDb;
    static const QHash<SettingCategory, QString> settingCategoryToIdForDb;
            // Don't use these maps directly. Use the methods above.
};


struct SettingBoxDataUpdate
{
    std::optional<QRectF> rect;

    QJsonObject toJson() const;
};

#endif // SETTINGBOXDATA_H
