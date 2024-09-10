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
    if (value.isUndefined()) {
        Q_ASSERT(false);
        return;
    }

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

QHash<QString, QJsonValue> Card::getCustomProperties() const {
    return customProperties;
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

    const auto customPropUpdate = propertiesUpdate.getCustomProperties();
    for (auto it = customPropUpdate.constBegin(); it != customPropUpdate.constEnd(); ++it) {
        if (it.value().isUndefined())
            customProperties.remove(it.key());
        else
            customProperties.insert(it.key(), it.value());
    }

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

QJsonObject CardPropertiesUpdate::toJson(const UndefinedHandlingOption option) const {
    QJsonObject obj;

    // custom properties
    for (auto it = customProperties.constBegin(); it != customProperties.constEnd(); ++it) {
        QJsonValue value = it.value();
        if (value.isUndefined()) {
            // handle Undefined value
            switch (option) {
            case UndefinedHandlingOption::ReplaceByNull:
                value = QJsonValue::Null;
                break;
            case UndefinedHandlingOption::ReplaceByRemoveString:
                value = "<Remove>";
                break;
            }

            if (value.isUndefined()) { // case not implemented
                Q_ASSERT(false);
                value = QJsonValue::Null;
            }
        }
        obj.insert(it.key(), value);
    }

    //
    if (title.has_value())
        obj.insert("title", title.value());

    if (text.has_value())
        obj.insert("text", text.value());

    if (tags.has_value())
        obj.insert("tags", toJsonArray(tags.value()));

    return obj;
}

void CardPropertiesUpdate::mergeWith(const CardPropertiesUpdate &other) {
#define UPDATE_ITEM(item) \
        if (other.item.has_value()) item = other.item;

    UPDATE_ITEM(title);
    UPDATE_ITEM(text);
    UPDATE_ITEM(tags);

#undef UPDATE_ITEM

    //
    for (auto it = other.customProperties.constBegin();
            it != other.customProperties.constEnd(); ++it) {
        customProperties.insert(it.key(), it.value());
    }
}
