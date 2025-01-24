#include "settings.h"

QVector<SettingTargetTypeAndCategory> getValidTargetTypeAndCategoryPairs() {
    return {
        {SettingTargetType::Workspace, SettingCategory::WorkspaceSchema},
        {SettingTargetType::Workspace, SettingCategory::CardLabelToColorMapping},
        {SettingTargetType::Workspace, SettingCategory::CardPropertiesToShow},
        {SettingTargetType::Board, SettingCategory::CardLabelToColorMapping},
        {SettingTargetType::Board, SettingCategory::CardPropertiesToShow},
    };
}

QString getDisplayNameOfTargetType(const SettingTargetType targetType) {
    switch (targetType) {
    case SettingTargetType::Workspace:
        return "Workspace";
    case SettingTargetType::Board:
        return "Board";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

QString getDisplayNameOfCategory(const SettingCategory category) {
    switch (category) {
    case SettingCategory::CardLabelToColorMapping:
        return "Card Label-Color Mapping";
    case SettingCategory::CardPropertiesToShow:
        return "Card Properties to Show";
    case SettingCategory::WorkspaceSchema:
        return "Schema";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

QString getDescriptionOfCategory(const SettingCategory category) {
    switch (category) {
    case SettingCategory::CardLabelToColorMapping:
        return "You can also set the mapping from the workspace menu.";
    case SettingCategory::CardPropertiesToShow:
        return "";
    case SettingCategory::WorkspaceSchema:
        return "";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

std::pair<QVector<SettingTargetTypeAndCategory>, QStringList>
getTargetTypeAndCategoryDisplayNames() {
    QVector<SettingTargetTypeAndCategory> targetTypeAndCategoryPairs
            = getValidTargetTypeAndCategoryPairs();

    QStringList displayNames;
    for (const auto &targetTypeAndCategory: qAsConst(targetTypeAndCategoryPairs)) {
        const QString targetTypeName = getDisplayNameOfTargetType(targetTypeAndCategory.first);
        const QString categoryName = getDisplayNameOfCategory(targetTypeAndCategory.second);
        displayNames << QString("%1 - %2").arg(targetTypeName, categoryName);
    }

    Q_ASSERT(targetTypeAndCategoryPairs.count() == displayNames.count());
    return {targetTypeAndCategoryPairs, displayNames};
}
