#ifndef CARD_PROPERTIES_TO_SHOW_H
#define CARD_PROPERTIES_TO_SHOW_H

#include "models/settings/abstract_setting.h"

struct ValueDisplayFormat
{
    // - A display string can contain '$', which will be replaced by the printed value.
    // - "$$" will be replaced with "$"

    QHash<QString, QString> caseValueToString;
    QString defaultStringIfExists {"$"}; // cannot be empty
    std::optional<QString> stringIfNotExists; // nullopt: don't display if not exists
    bool hideLabel {false};
    bool addQuotesForString {false};

    //
    QJsonObject toJson() const;
    static std::optional<ValueDisplayFormat> fromJson(
            const QJsonValue &v, QString *errorMsg = nullptr);
};


struct CardPropertiesToShow : public AbstractWorkspaceOrBoardSetting
{
public:
    explicit CardPropertiesToShow();

    QVector<std::pair<QString, ValueDisplayFormat>> propertyNamesAndDisplayFormats;

    //
    QString toJsonStr(const QJsonDocument::JsonFormat format) const override;
    QString schema() const override;
    bool validate(const QString &s, QString *errorMsg = nullptr) override;
    bool setFromJsonStr(const QString &jsonStr) override;

    static std::optional<CardPropertiesToShow> fromJsonStr(
            const QString &jsonStr, QString *errorMsg = nullptr);
};

#endif // CARD_PROPERTIES_TO_SHOW_H
