#include <QDebug>
#include <QJsonArray>
#include "setting_box_data.h"
#include "utilities/json_util.h"

const QHash<SettingTargetType, QString> SettingBoxData::settingTargetTypeToIdForDb {
    {SettingTargetType::Workspace, "workspace"},
    {SettingTargetType::Board, "board"},
};

const QHash<SettingCategory, QString> SettingBoxData::settingCategoryToIdForDb {
    {SettingCategory::WorkspaceSchema, "schema"},
    {SettingCategory::CardLabelToColorMapping, "cardLabelToColorMapping"},
    {SettingCategory::CardPropertiesToShow, "cardPropertiesToShow"},
};

QString SettingBoxData::getTargetTypeId() const {
    return getSettingTargetTypeIdForDb(targetType);
}

QString SettingBoxData::getCategoryId() const {
    return getSettingCategoryIdForDb(category);
}

QJsonObject SettingBoxData::toJson() const {
    QJsonObject obj;
    obj.insert("targetType", getSettingTargetTypeIdForDb(targetType));
    obj.insert("category", getSettingCategoryIdForDb(category));
    obj.insert("rect", QJsonArray {rect.x(), rect.y(), rect.width(), rect.height()});
    return obj;
}

std::optional<SettingBoxData> SettingBoxData::fromJson(const QJsonObject &obj) {
    SettingBoxData data;

    {
        const QJsonValue targetTypeValue = obj.value("targetType");
        if (!targetTypeValue.isString())
            return std::nullopt;

        const QString targetTypeId = targetTypeValue.toString();
        const std::optional<SettingTargetType> targetTypeOpt
                = getSettingTargetTypeFromIdForDb(targetTypeId);
        if (!targetTypeOpt.has_value()) {
            qWarning().noquote()
                    << QString("unrecognized setting target-type ID %1").arg(targetTypeId);
            return std::nullopt;
        }

        data.targetType = targetTypeOpt.value();
    }
    {
        const QJsonValue categoryValue = obj.value("category");
        if (!categoryValue.isString())
            return std::nullopt;

        const QString categoryId = categoryValue.toString();
        const std::optional<SettingCategory> categoryOpt = getSettingCategoryFromIdForDb(categoryId);
        if (!categoryOpt.has_value()) {
            qWarning().noquote() << QString("unrecognized setting category ID %1").arg(categoryId);
            return std::nullopt;
        }

        data.category = categoryOpt.value();
    }
    {
        const QJsonValue rectValue = obj.value("rect");
        if (!jsonValueIsArrayOfSize(rectValue, 4)) {
            qWarning().noquote() << QString("unrecognized \"rect\" value");
            return std::nullopt;
        }

        data.rect = QRectF(
                QPointF {rectValue[0].toDouble(), rectValue[1].toDouble()},
                QSizeF {rectValue[2].toDouble(), rectValue[3].toDouble()}
        );
    }

    return data;
}

void SettingBoxData::update(const SettingBoxDataUpdate &update) {
#define UPDATE_ITEM(item) \
    if (update.item.has_value()) \
        this->item = update.item.value();

    UPDATE_ITEM(rect);

#undef UPDATE_ITEM
}

QString SettingBoxData::getSettingTargetTypeIdForDb(const SettingTargetType targetType) {
    if (settingTargetTypeToIdForDb.contains(targetType)) {
        return settingTargetTypeToIdForDb.value(targetType);
    }
    else {
        Q_ASSERT(false); // key is missing in `settingTargetTypeToIdForDb`
        return "";
    }
}

QString SettingBoxData::getSettingCategoryIdForDb(const SettingCategory category) {
    if (settingCategoryToIdForDb.contains(category)) {
        return settingCategoryToIdForDb.value(category);
    }
    else {
        Q_ASSERT(false); // key is missing in `settingCategoryToIdForDb`
        return "";
    }
}

std::optional<SettingTargetType> SettingBoxData::getSettingTargetTypeFromIdForDb(const QString &id) {
    static QHash<QString, SettingTargetType> settingTargetTypeById;
    if (settingTargetTypeById.isEmpty()) {
        for (auto it = settingTargetTypeToIdForDb.constBegin();
                it != settingTargetTypeToIdForDb.constEnd(); ++it) {
            settingTargetTypeById.insert(it.value(), it.key());
        }
    }

    if (settingTargetTypeById.contains(id))
        return settingTargetTypeById.value(id);
    else
        return std::nullopt;
}

std::optional<SettingCategory> SettingBoxData::getSettingCategoryFromIdForDb(const QString &id) {
    static QHash<QString, SettingCategory> settingCategoryById;
    if (settingCategoryById.isEmpty()) {
        for (auto it = settingCategoryToIdForDb.constBegin();
                it != settingCategoryToIdForDb.constEnd(); ++it) {
            settingCategoryById.insert(it.value(), it.key());
        }
    }

    if (settingCategoryById.contains(id))
        return settingCategoryById.value(id);
    else
        return std::nullopt;
}

QJsonObject SettingBoxDataUpdate::toJson() const {
    QJsonObject obj;

    if (rect.has_value()) {
        const QRectF &r = rect.value();
        obj.insert("rect", QJsonArray {r.x(), r.y(), r.width(), r.height()});
    }

    return obj;
}
