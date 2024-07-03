#ifndef BOARD_H
#define BOARD_H

#include <QHash>
#include <QString>
#include "relationship.h"

struct Board {
    QString name;

    QHash<int, int> cardsIdToVisualNodeId;
//    QHash<int, VisualNodeData> visualNodesIdToData;

    QHash<RelationshipId, int> relsIdToVisualEdgeId;
//    QHash<int, VisualEdgeData> visualEdgesIdToData;
};

#endif // BOARD_H
