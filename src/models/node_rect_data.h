#ifndef NODE_RECT_DATA_H
#define NODE_RECT_DATA_H

#include <optional>
#include <QColor>
#include <QJsonObject>
#include <QRectF>

struct NodeRectDataUpdate;

struct NodeRectData
{
    QRectF rect;
    QColor ownColor;

    QJsonObject toJson() const;
    static std::optional<NodeRectData> fromJson(const QJsonObject &obj);
    void update(const NodeRectDataUpdate &update);
};


struct NodeRectDataUpdate
{
    std::optional<QRectF> rect;
    std::optional<QColor> ownColor;

    QJsonObject toJson() const;
    QSet<QString> keys() const;
};

#endif // NODE_RECT_DATA_H
