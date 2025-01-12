#include <QJsonArray>
#include "group_box_data.h"

namespace {
QJsonArray rectToJsonArray(const QRectF &rect);
std::optional<QRectF> parseJsonValueAsRect(const QJsonValue &v);
}

QJsonObject GroupBoxData::toJson() const {
    return QJsonObject {
        {"title", title},
        {"rect", rectToJsonArray(rect)}
    };
}

std::optional<GroupBoxData> GroupBoxData::fromJson(const QJsonObject &obj) {
    GroupBoxData data;

    if (const QJsonValue v = obj.value("title"); !v.isUndefined())
        data.title = v.toString();

    if (const QJsonValue v = obj.value("rect"); !v.isUndefined()) {
        const auto rectOpt = parseJsonValueAsRect(v);
        if (!rectOpt.has_value())
            return std::nullopt;
        data.rect = rectOpt.value();
    }

    return data;
}

void GroupBoxData::update(const GroupBoxDataUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(title);
    UPDATE_PROPERTY(rect);

#undef UPDATE_PROPERTY
}

//======

QJsonObject GroupBoxDataUpdate::toJson() {
    QJsonObject obj;

    if (title.has_value())
        obj.insert("title", title.value());
    if (rect.has_value())
        obj.insert("rect", rectToJsonArray(rect.value()));

    return obj;
}

//======

namespace {
QJsonArray rectToJsonArray(const QRectF &rect) {
    return QJsonArray {rect.x(), rect.y(), rect.width(), rect.height()};
}

std::optional<QRectF> parseJsonValueAsRect(const QJsonValue &v) {
    if (!v.isArray())
        return std::nullopt;
    const QJsonArray array = v.toArray();
    if (array.count() != 4)
        return std::nullopt;
    return QRectF(
            array.at(0).toDouble(), array.at(1).toDouble(),
            array.at(2).toDouble(), array.at(3).toDouble());
}
} // namespace

