#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QColor>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>
#include "models/workspace_board_settings/card_label_color_mapping_setting.h"

struct WorkspaceNodePropertiesUpdate;

struct Workspace {
    // properties of `Workspace` node
    QString name;
    QVector<int> boardsOrdering;
    int lastOpenedBoardId {-1};

    CardLabelToColorMapping cardLabelToColorMappingSetting;

    //
    QSet<int> boardIds;

    //
    QJsonObject getNodePropertiesJson() const;

    void updateNodeProperties(const QJsonObject &obj);
    void updateNodeProperties(const WorkspaceNodePropertiesUpdate &update);
};


struct WorkspaceNodePropertiesUpdate {
    std::optional<QString> name;
    std::optional<QVector<int>> boardsOrdering;
    std::optional<int> lastOpenedBoardId;
    std::optional<CardLabelToColorMapping> cardLabelToColorMappingSetting;

    QJsonObject toJson() const;
};

#endif // WORKSPACE_H
