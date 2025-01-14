#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include "models/data_view_box_data.h"
#include "models/group_box_data.h"
#include "models/node_rect_data.h"
#include "relationship.h"

struct BoardNodePropertiesUpdate;

struct Board {
    // properties of `Board` node
    QString name;

    QPointF topLeftPos {0, 0}; // view's top-left corner
    double zoomRatio {1.0};

    //
    QHash<int, NodeRectData> cardIdToNodeRectData;
    QHash<int, DataViewBoxData> customDataQueryIdToDataViewBoxData;
    QHash<int, GroupBoxData> groupBoxIdToData; // including all descendant group-boxes of the board

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const BoardNodePropertiesUpdate &update);

    // tools
    int findParentOfGroupBox(const int groupBoxId) const; // returns -1 if not found
};


struct BoardNodePropertiesUpdate
{
    std::optional<QString> name;
    std::optional<QPointF> topLeftPos;
    std::optional<double> zoomRatio;

    QJsonObject toJson() const;
    QSet<QString> keys() const;
};

#endif // BOARD_H
