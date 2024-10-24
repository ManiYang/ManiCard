#ifndef DATA_VIEW_BOX_DATA_H
#define DATA_VIEW_BOX_DATA_H

#include <optional>
#include <QColor>
#include <QJsonObject>
#include <QRectF>

struct DataViewBoxData
{
    QRectF rect;
    QColor ownColor;

    static std::optional<DataViewBoxData> fromJson(const QJsonObject &obj);
};

#endif // DATA_VIEW_BOX_DATA_H
