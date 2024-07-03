#include "card.h"
#include "node_labels.h"
#include "utilities/json_util.h"

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

QJsonObject Card::getPropertiesJson() const {
    QJsonObject obj;

    obj.insert("title", title);
    obj.insert("text", text);
    obj.insert("tags", toJsonArray(tags));

    return obj;
}

Card &Card::updateProperties(const QJsonObject &obj) {
    if (const auto v = obj.value("title"); !v.isUndefined())
        title = v.toString();
    if (const auto v = obj.value("text"); !v.isUndefined())
        text = v.toString();
    if (const auto v = obj.value("tags"); !v.isUndefined())
        tags = toStringList(v.toArray(), "");

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

    return *this;
}

//====

QJsonObject CardPropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (title.has_value())
        obj.insert("title", title.value());

    if (text.has_value())
        obj.insert("text", text.value());

    if (tags.has_value())
        obj.insert("tags", toJsonArray(tags.value()));

    return obj;
}
