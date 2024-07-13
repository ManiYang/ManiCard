#include <QJsonArray>
#include "board.h"

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}}
    };
}

void Board::updateNodeProperties(const QJsonObject &obj) {
    if (const auto v = obj.value("name"); !v.isUndefined())
        name = v.toString();

    if (const auto v = obj.value("topLeftPos"); !v.isUndefined()) {
        const QJsonArray array = v.toArray();
        if (array.count() == 2)
            topLeftPos = QPointF(array.at(0).toDouble(), array.at(1).toDouble());
        else
            topLeftPos = QPointF(0, 0);
    }
}

void Board::updateNodeProperties(const BoardNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(topLeftPos);

#undef UPDATE_PROPERTY
}

//====

QJsonObject BoardNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (topLeftPos.has_value())
        obj.insert("topLeftPos", QJsonArray{topLeftPos.value().x(), topLeftPos.value().y()});

    return obj;
}
