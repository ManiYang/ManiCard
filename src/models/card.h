#ifndef CARD_H
#define CARD_H

#include <optional>
#include <QJsonObject>
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

    QJsonObject getPropertiesJson() const;

    Card &updateProperties(const QJsonObject &obj);
    Card &updateProperties(const CardPropertiesUpdate &propertiesUpdate);

private:
    QSet<QString> labels; // not including "Card"
};

struct CardPropertiesUpdate
{
    std::optional<QString> title;
    std::optional<QString> text;
    std::optional<QStringList> tags;

    QJsonObject toJson() const;
};

#endif // CARD_H
