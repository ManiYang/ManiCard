#ifndef DATA_VIEW_BOX_DATA_H
#define DATA_VIEW_BOX_DATA_H

#include <optional>
#include <QColor>
#include <QJsonObject>
#include <QRectF>

struct DataViewBoxDataUpdate;

struct DataViewBoxData
{
    QRectF rect;
    QColor ownColor;

    //
    void update(const DataViewBoxDataUpdate &update);

    QJsonObject toJson() const;
    static std::optional<DataViewBoxData> fromJson(const QJsonObject &obj);
};

struct DataViewBoxDataUpdate
{
    std::optional<QRectF> rect;
    std::optional<QColor> ownColor;

    //
    QJsonObject toJson() const;
};

#endif // DATA_VIEW_BOX_DATA_H
