#ifndef CARD_PROPERTIES_TO_SHOW_H
#define CARD_PROPERTIES_TO_SHOW_H

#include <optional>
#include "models/settings/abstract_setting.h"

struct ValueDisplayFormat
{
    // For `caseValueToString` & `defaultStringIfExists`:
    // - A display string can contain '$', which will be replaced by the printed value.
    // - "$$" will be replaced with "$".

    QHash<QString, QString> caseValueToString;
    QString defaultStringIfExists {"$"}; // cannot be empty
    std::optional<QString> stringIfNotExists; // nullopt: don't display if not exists
    bool hideLabel {false}; // hide property-name label?
    bool addQuotesForString {false};

    //!
    //! \param value: the value of an existing property. Cannot be \c undefined.
    //! \return the display text of the value, not including the property-name label
    //!
    QString getValueDisplayText(const QJsonValue &value) const;

    //
    QJsonObject toJson() const;
    static std::optional<ValueDisplayFormat> fromJson(
            const QJsonValue &v, QString *errorMsg = nullptr);
};


struct CardPropertiesToShow : public AbstractWorkspaceOrBoardSetting
{
public:
    explicit CardPropertiesToShow();

    using PropertiesAndDisplayFormats = QVector<std::pair<QString, ValueDisplayFormat>>;

    //!
    //! \return the setting for a card with labels `cardLabels`
    //!
    PropertiesAndDisplayFormats getPropertiesToShow(const QSet<QString> &cardLabels) const;

    //
    void updateWith(const CardPropertiesToShow &other);

    //
    QString toJsonStr(const QJsonDocument::JsonFormat format) const override;
    QString schema() const override;
    bool validate(const QString &s, QString *errorMsg = nullptr) override;
    bool setFromJsonStr(const QString &jsonStr) override;

    static std::optional<CardPropertiesToShow> fromJsonStr(
            const QString &jsonStr, QString *errorMsg = nullptr);

    CardPropertiesToShow &operator = (const CardPropertiesToShow &other);

private:


//    QVector<std::pair<QString, PropertiesAndDisplayFormats>> cardLabelsAndSettings;
    QHash<QString, PropertiesAndDisplayFormats> cardLabelToSetting;
    QVector<QString> cardLabelsOrdering;

    static PropertiesAndDisplayFormats merge(
            const PropertiesAndDisplayFormats &vec1, const PropertiesAndDisplayFormats &vec2);
};

#endif // CARD_PROPERTIES_TO_SHOW_H
