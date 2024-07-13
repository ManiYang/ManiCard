#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include "models/node_rect_data.h"
#include "relationship.h"

struct Board {
    // properties of Board node in DB
    QString name;
    QPointF topLeftPos; // in scene coordinates

    //
    QHash<int, NodeRectData> cardIdToNodeRectData; // key: NodeRect ID

//    QHash<RelationshipId, int> relsIdToEdgeArrowId;
//    QHash<int, VisualEdgeData> visualEdgesIdToData;

    //
    void setNodeProperties(const QJsonObject &obj);
    QJsonObject getNodePropertiesJson() const;
};


struct BoardNodePropertyUpdate
{
    std::optional<QString> name;
    std::optional<QPointF> topLeftPos;

    QJsonObject toJson() const;
};

#endif // BOARD_H
