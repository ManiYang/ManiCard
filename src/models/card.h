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

    void insertCustomProperty(const QString &name, const QJsonValue &value);
    void removeCustomProperty(const QString &name);
    QSet<QString> getCustomPropertyNames() const;
    QJsonValue getCustomPropertyValue(const QString &name) const; // returns Undefined if not found

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
            // - should not contain "title", "text", "tags", "id"

};

struct CardPropertiesUpdate
{
    std::optional<QString> title;
    std::optional<QString> text;
    std::optional<QStringList> tags;

    void setCustomProperties(const QHash<QString, QJsonValue> &properties);
    QHash<QString, QJsonValue> getCustomProperties() const;

    QJsonObject toJson() const;

private:
    QHash<QString, QJsonValue> customProperties;
            // - keys are property names
            // - should not contain "title", "text", "tags", "id"
};

#endif // CARD_H
