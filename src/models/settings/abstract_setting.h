#ifndef ABSTRACT_SETTING_H
#define ABSTRACT_SETTING_H

#include <QJsonObject>
#include "models/settings/settings.h"

struct AbstractWorkspaceOrBoardSetting
{
    explicit AbstractWorkspaceOrBoardSetting(const SettingCategory category)
        : category(category) {}
    virtual ~AbstractWorkspaceOrBoardSetting() {}

    const SettingCategory category;

    virtual QJsonObject toJson() const = 0;
    virtual void setFromJson(const QJsonObject &obj) = 0;
};

#endif // ABSTRACT_SETTING_H
