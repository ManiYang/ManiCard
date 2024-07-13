#include <QJsonArray>
#include "node_rect_data.h"
#include "utilities/json_util.h"

QJsonObject NodeRectData::toJson() const {
    return {
        {"rect", QJsonArray {rect.left(), rect.top(), rect.width(), rect.height()}},
        {"color", color.name(QColor::HexRgb)}
    };
}

std::optional<NodeRectData> NodeRectData::fromJson(const QJsonObject &obj) {
    QVector<double> rectLeftTopWidthHeight;
    {
        const auto rectArray = obj.value("rect").toArray();
        if (rectArray.count() != 4)
            return std::nullopt;

        rectLeftTopWidthHeight = toDoubleVector(rectArray, 0.0);
    }

    NodeRectData data;
    data.rect = QRectF(
            rectLeftTopWidthHeight.at(0), rectLeftTopWidthHeight.at(1),
            rectLeftTopWidthHeight.at(2), rectLeftTopWidthHeight.at(3));
    data.color = QColor(obj.value("color").toString());

    return data;
}
