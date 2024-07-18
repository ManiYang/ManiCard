#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include "models/node_rect_data.h"
#include "relationship.h"

struct BoardNodePropertiesUpdate;

struct Board {
    // properties of Board node in DB
    QString name;
    QPointF topLeftPos; // in scene coordinates

    //
    QHash<int, NodeRectData> cardIdToNodeRectData;
//    QHash<RelationshipId, EdgeArrowData> relsIdToEdgeArrowData;

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const BoardNodePropertiesUpdate &update);
};


struct BoardNodePropertiesUpdate
{
    std::optional<QString> name;
    std::optional<QPointF> topLeftPos;

    QJsonObject toJson() const;
};

#endif // BOARD_H
