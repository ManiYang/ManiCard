#ifndef CARD_PROPERTIES_TO_SHOW_H
#define CARD_PROPERTIES_TO_SHOW_H

#include "models/settings/abstract_setting.h"

struct CardPropertiesToShow : public AbstractWorkspaceOrBoardSetting
{
public:
    explicit CardPropertiesToShow();

    //
    QString toJsonStr(const QJsonDocument::JsonFormat format) const override;
    QString schema() const override;
    bool validate(const QString &s, QString *errorMsg = nullptr) override;
};

#endif // CARD_PROPERTIES_TO_SHOW_H
