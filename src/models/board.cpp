#include <QJsonArray>
#include "board.h"
#include "utilities/json_util.h"

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}},
        {"zoomRatio", zoomRatio},
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

    if (const auto v = obj.value("zoomRatio"); !v.isUndefined()) {
        zoomRatio = v.toDouble(1.0);
    }
}

void Board::updateNodeProperties(const BoardNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(topLeftPos);
    UPDATE_PROPERTY(zoomRatio);

#undef UPDATE_PROPERTY
}

int Board::findParentOfGroupBox(const int groupBoxId) const {
    for (auto it = groupBoxIdToData.constBegin(); it != groupBoxIdToData.constEnd(); ++it) {
        if (it.value().childGroupBoxes.contains(groupBoxId))
            return it.key();
    }
    return -1;
}

//====

QJsonObject BoardNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (topLeftPos.has_value())
        obj.insert("topLeftPos", QJsonArray{topLeftPos.value().x(), topLeftPos.value().y()});

    if (zoomRatio.has_value())
        obj.insert("zoomRatio", zoomRatio.value());

    return obj;
}

QSet<QString> BoardNodePropertiesUpdate::keys() const {
    return keySet(toJson());
}
