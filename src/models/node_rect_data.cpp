#include <QDebug>
#include <QJsonArray>
#include "node_rect_data.h"
#include "utilities/json_util.h"

QJsonObject NodeRectData::toJson() const {
    QJsonObject obj;

    obj.insert("rect", QJsonArray {rect.left(), rect.top(), rect.width(), rect.height()});

    if (ownColor.isValid())
        obj.insert("ownColor", ownColor.name(QColor::HexRgb));

    return obj;
}

std::optional<NodeRectData> NodeRectData::fromJson(const QJsonObject &obj) {
    QVector<double> rectLeftTopWidthHeight;
    {
        const auto rectArray = obj.value("rect").toArray();
        if (rectArray.count() != 4) {
            qWarning().noquote() << "obj[\"rect\"] is not an array of size 4";
            return std::nullopt;
        }

        rectLeftTopWidthHeight = toDoubleVector(rectArray, 0.0);
    }

    NodeRectData data;
    data.rect = QRectF(
            rectLeftTopWidthHeight.at(0), rectLeftTopWidthHeight.at(1),
            rectLeftTopWidthHeight.at(2), rectLeftTopWidthHeight.at(3));
    data.ownColor = QColor(obj.value("ownColor").toString()); // invalid color if key not found

    return data;
}

void NodeRectData::update(const NodeRectDataUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(rect);
    UPDATE_PROPERTY(ownColor);

#undef UPDATE_PROPERTY
}

//====

QJsonObject NodeRectDataUpdate::toJson() const {
    QJsonObject obj;

    if (rect.has_value()) {
        obj.insert(
                "rect",
                QJsonArray {
                    rect.value().x(), rect.value().y(),
                    rect.value().width(), rect.value().height()
                }
        );
    }

    if (ownColor.has_value() && ownColor.value().isValid())
        obj.insert("ownColor", ownColor.value().name(QColor::HexRgb));

    return obj;
}

QSet<QString> NodeRectDataUpdate::keys() const {
    return keySet(toJson());
}
