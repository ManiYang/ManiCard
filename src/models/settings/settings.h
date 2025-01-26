#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QVector>
#include "utilities/hash.h"

enum class SettingTargetType {Workspace, Board};

enum class SettingCategory {
    CardLabelToColorMapping,
    CardPropertiesToShow,
    WorkspaceSchema // for workspace only
};

using SettingTargetTypeAndCategory = std::pair<SettingTargetType, SettingCategory>;

inline uint qHash(const SettingTargetType &targetType, uint seed) {
    return qHash(static_cast<int>(targetType), seed);
}

inline uint qHash(const SettingCategory &category, uint seed) {
    return qHash(static_cast<int>(category), seed);
}

inline uint qHash(const SettingTargetTypeAndCategory &targetTypeAndCategory, uint seed) {
    hashCombine(seed, static_cast<int>(targetTypeAndCategory.first));
    hashCombine(seed, static_cast<int>(targetTypeAndCategory.second));
    return seed;
}

//

QVector<SettingTargetTypeAndCategory> getValidTargetTypeAndCategoryPairs();

QString getDisplayNameOfTargetType(const SettingTargetType targetType);

QString getDisplayNameOfCategory(const SettingCategory category);

QString getDescriptionForTargetTypeAndCategory(
        const SettingTargetTypeAndCategory &targetTypeAndCategory);

//!
//! \return (targetTypeAndCategoryPairs, displayNames), where displayNames[i] corresponds to
//!         targetTypeAndCategoryPairs[i]
//!
std::pair<QVector<SettingTargetTypeAndCategory>, QStringList> getTargetTypeAndCategoryDisplayNames();

#endif // SETTINGS_H
