#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QVector>

enum class SettingTargetType {Workspace, Board};

enum class SettingCategory {
    CardLabelToColorMapping,
    CardPropertiesToShow,
    WorkspaceSchema // for workspace only
};

inline uint qHash(const SettingCategory &category, uint seed) {
    return qHash(static_cast<int>(category), seed);
}

using SettingTargetTypeAndCategory = std::pair<SettingTargetType, SettingCategory>;
QVector<SettingTargetTypeAndCategory> getValidTargetTypeAndCategoryPairs();

QString getDisplayNameOfTargetType(const SettingTargetType targetType);

QString getDisplayNameOfCategory(const SettingCategory category);

QString getDescriptionOfCategory(const SettingCategory category);

//!
//! \return (targetTypeAndCategoryPairs, displayNames), where displayNames[i] corresponds to
//!         targetTypeAndCategoryPairs[i]
//!
std::pair<QVector<SettingTargetTypeAndCategory>, QStringList> getTargetTypeAndCategoryDisplayNames();

#endif // SETTINGS_H
