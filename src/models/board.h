#ifndef BOARD_H
#define BOARD_H

#include <optional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include "models/data_view_box_data.h"
#include "models/group_box_data.h"
#include "models/node_rect_data.h"
#include "models/setting_box_data.h"
#include "models/settings/card_properties_to_show.h"
#include "relationship.h"

struct BoardNodePropertiesUpdate;

struct Board {
    // properties of `Board` node
    QString name;

    QPointF topLeftPos {0, 0}; // view's top-left corner
    double zoomRatio {1.0};

    CardPropertiesToShow cardPropertiesToShow;

    //
    QHash<int, NodeRectData> cardIdToNodeRectData;
    QHash<int, DataViewBoxData> customDataQueryIdToDataViewBoxData;
    QHash<int, GroupBoxData> groupBoxIdToData;
            // includes all group-boxes the board has (directly or indirectly)
    QVector<SettingBoxData> settingBoxesData;

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const BoardNodePropertiesUpdate &update);

    // tools
    int findParentGroupBoxOfGroupBox(const int groupBoxId) const; // returns -1 if not found
    int findParentGroupBoxOfCard(const int cardId) const; // returns -1 if not found
    bool isGroupBoxADescendantOfGroupBox(const int groupBoxId1, const int groupBoxId2) const;
            // + returns false if either group box is not found
            // + returns false if `groupBoxId1` = `groupBoxId2`

    bool hasSettingBoxFor(const SettingTargetType targetType, const SettingCategory category) const;
    void updateSettingBoxData(
            const SettingTargetType targetType, const SettingCategory category,
            const SettingBoxDataUpdate &update); // ignored if not found
    void removeSettingBoxData(const SettingTargetType targetType, const SettingCategory category);
            // ignored if not found
};


struct BoardNodePropertiesUpdate
{
    std::optional<QString> name;
    std::optional<QPointF> topLeftPos;
    std::optional<double> zoomRatio;
    std::optional<CardPropertiesToShow> cardPropertiesToShow;

    QJsonObject toJson() const;
    QSet<QString> keys() const;
};

#endif // BOARD_H
