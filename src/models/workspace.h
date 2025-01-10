#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

struct WorkspaceNodePropertiesUpdate;

struct Workspace {
    // properties of `Workspace` node
    QString name;
    QVector<int> boardsOrdering;
    int lastOpenedBoardId {-1};

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

    QJsonObject toJson() const;
};

#endif // WORKSPACE_H
