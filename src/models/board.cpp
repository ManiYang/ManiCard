#include <QJsonArray>
#include "board.h"

void Board::setNodeProperties(const QJsonObject &obj) {
    name = obj.value("name").toString();

    const QJsonArray topLeftPosArray = obj.value("topLeftPos").toArray();
    if (topLeftPosArray.count() == 2) {
        topLeftPos
                = QPointF(topLeftPosArray.at(0).toDouble(), topLeftPosArray.at(1).toDouble());
    }
    else {
        topLeftPos = QPointF(0, 0);
    }
}

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}}
    };
}

//====

QJsonObject BoardNodePropertyUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (topLeftPos.has_value())
        obj.insert("topLeftPos", QJsonArray{topLeftPos.value().x(), topLeftPos.value().y()});

    return obj;
}
