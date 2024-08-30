#include "card.h"
#include "node_labels.h"
#include "utilities/json_util.h"
#include "utilities/maps_util.h"

Card &Card::addLabels(const QSet<QString> &labelsToAdd) {
    labels += labelsToAdd;
    labels -= NodeLabel::card;
    return *this;
}

Card &Card::addLabels(const QStringList &labelsToAdd) {
    for (const QString &label: labelsToAdd)
        labels += label;
    labels -= NodeLabel::card;
    return *this;
}

Card &Card::setLabels(const QSet<QString> &labels_) {
    labels = labels_;
    labels -= NodeLabel::card;
    return *this;
}

QSet<QString> Card::getLabels() const {
    return labels;
}

void Card::insertCustomProperty(const QString &name, const QJsonValue &value) {
    const static QSet<QString> disallowedNames {"title", "text", "tags", "id"};
    if (disallowedNames.contains(name)) {
        Q_ASSERT(false);
        return;
    }
    customProperties.insert(name, value);
}

void Card::removeCustomProperty(const QString &name) {
    customProperties.remove(name);
}

QSet<QString> Card::getCustomPropertyNames() const {
    return keySet(customProperties);
}

QJsonValue Card::getCustomPropertyValue(const QString &name) const {
    return customProperties.value(name, QJsonValue::Undefined);
}

QJsonObject Card::getPropertiesJson() const {
    QJsonObject obj;

    for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());

    obj.insert("title", title);
    obj.insert("text", text);
    obj.insert("tags", toJsonArray(tags));

    return obj;
}

Card &Card::updateProperties(const QJsonObject &obj, const bool ignoreId) {
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString name = it.key();
        const QJsonValue value = it.value();

        if (name == "id" && ignoreId)
            continue;

        if (name == "title")
            title = value.toString();
        else if (name == "text")
            text = value.toString();
        else if (name == "tags")
            tags = toStringList(value.toArray(), "");
        else
            customProperties.insert(name, value);
    }

    return *this;
}

Card &Card::updateProperties(const CardPropertiesUpdate &propertiesUpdate) {
#define UPDATE_PROPERTY(member) \
        if (propertiesUpdate.member.has_value()) \
            this->member = propertiesUpdate.member.value();

    UPDATE_PROPERTY(title);
    UPDATE_PROPERTY(text);
    UPDATE_PROPERTY(tags);
#undef UPDATE_PROPERTY

    mergeWith(customProperties, propertiesUpdate.getCustomProperties());

    return *this;
}

//====

void CardPropertiesUpdate::setCustomProperties(const QHash<QString, QJsonValue> &properties) {
    const static QSet<QString> disallowedNames {"title", "text", "tags", "id"};

    customProperties.clear();
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        const QString &name = it.key();
        if (disallowedNames.contains(name)) {
            Q_ASSERT(false);
            continue;
        }
        customProperties.insert(name, it.value());
    }
}

QHash<QString, QJsonValue> CardPropertiesUpdate::getCustomProperties() const {
    return customProperties;
}

QJsonObject CardPropertiesUpdate::toJson() const {
    QJsonObject obj;

    for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it)
        obj.insert(it.key(), it.value());

    if (title.has_value())
        obj.insert("title", title.value());

    if (text.has_value())
        obj.insert("text", text.value());

    if (tags.has_value())
        obj.insert("tags", toJsonArray(tags.value()));

    return obj;
}
