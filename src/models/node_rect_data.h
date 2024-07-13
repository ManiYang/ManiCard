#ifndef NODE_RECT_DATA_H
#define NODE_RECT_DATA_H

#include <optional>
#include <QColor>
#include <QJsonObject>
#include <QRectF>

struct NodeRectData
{
    QRectF rect;
    QColor color;

    QJsonObject toJson() const;
    static std::optional<NodeRectData> fromJson(const QJsonObject &obj);
};

#endif // NODE_RECT_DATA_H
