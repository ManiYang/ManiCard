#ifndef CARD_H
#define CARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QString>
#include <QStringList>

struct CardPropertiesUpdate;

struct Card
{
    // ==== labels ====

    Card &addLabels(const QSet<QString> &labelsToAdd);
    Card &addLabels(const QStringList &labelsToAdd);
    Card &setLabels(const QSet<QString> &labels);

    //!
    //! \return labels other than "Card"
    //!
    QSet<QString> getLabels() const;

    // ==== properties ====

    QString title;
    QString text;
    QStringList tags;

    void insertCustomProperty(
            const QString &name, const QJsonValue &value); // `value` cannot be Undefined
    void removeCustomProperty(const QString &name);
    QHash<QString, QJsonValue> getCustomProperties() const;

    QJsonObject getPropertiesJson() const;

    //!
    //! \param obj
    //! \param ignoreId: if true, will ignore obj["id"]
    //!
    Card &updateProperties(const QJsonObject &obj, const bool ignoreId = true);

    Card &updateProperties(const CardPropertiesUpdate &propertiesUpdate);

private:
    QSet<QString> labels; // not including "Card"
    QHash<QString, QJsonValue> customProperties;
            // - keys are property names
            // - key cannot not be "title", "text", "tags", "id"
            // - value cannot be Undefined
};

struct CardPropertiesUpdate
{
    std::optional<QString> title;
    std::optional<QString> text;
    std::optional<QStringList> tags;

    //!
    //! A key (property name) can have Undefined value, meaning the removal of property.
    //!
    void setCustomProperties(const QHash<QString, QJsonValue> &properties);

    //!
    //! A key (property name) can have Undefined value, meaning the removal of property.
    //!
    QHash<QString, QJsonValue> getCustomProperties() const;

    enum class UndefinedHandlingOption {
        ReplaceByNull,
        ReplaceByRemoveString // replace by string "<Remove>"
    };
    QJsonObject toJson(
            const UndefinedHandlingOption option = UndefinedHandlingOption::ReplaceByNull) const;

private:
    QHash<QString, QJsonValue> customProperties;
            // - keys are property names
            // - keys cannot contain "title", "text", "tags", "id"
            // - a key can have Undefined value, meaning the removal of the property
};

#endif // CARD_H
