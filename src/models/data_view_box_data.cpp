#include <QDebug>
#include <QJsonArray>
#include "data_view_box_data.h"
#include "utilities/json_util.h"

std::optional<DataViewBoxData> DataViewBoxData::fromJson(const QJsonObject &obj) {
    QVector<double> rectLeftTopWidthHeight;
    {
        const auto rectArray = obj.value("rect").toArray();
        if (rectArray.count() != 4) {
            qWarning().noquote() << "obj[\"rect\"] is not an array of size 4";
            return std::nullopt;
        }

        rectLeftTopWidthHeight = toDoubleVector(rectArray, 0.0);
    }

    DataViewBoxData data;
    data.rect = QRectF(
            rectLeftTopWidthHeight.at(0), rectLeftTopWidthHeight.at(1),
            rectLeftTopWidthHeight.at(2), rectLeftTopWidthHeight.at(3));
    data.ownColor = QColor(obj.value("ownColor").toString()); // invalid color if key not found

    return data;
}
