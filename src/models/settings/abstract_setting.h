#ifndef ABSTRACT_SETTING_H
#define ABSTRACT_SETTING_H

#include <QJsonDocument>
#include <QString>
#include "models/settings/settings.h"

struct AbstractWorkspaceOrBoardSetting
{
    explicit AbstractWorkspaceOrBoardSetting(const SettingCategory category)
        : category(category) {}
    virtual ~AbstractWorkspaceOrBoardSetting() {}

    const SettingCategory category;

    virtual QString toJsonStr(const QJsonDocument::JsonFormat format) const = 0;
    virtual QString schema() const = 0;
    virtual bool validate(const QString &s, QString *errorMsg = nullptr) = 0;
    virtual bool setFromJsonStr(const QString &jsonStr) = 0;

protected:
    // tools
    static QString removeCommonIndentation(const QString &s);
};

#endif // ABSTRACT_SETTING_H
