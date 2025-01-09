#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

struct Workspace {
    // properties of `Workspace` node
    QString name;
    QVector<int> boardsOrdering;
    int lastOpenedBoardId {-1};

    //
    QSet<int> boardIds;

    //
    void updateNodeProperties(const QJsonObject &obj);
};


//struct WorkspaceNodePropertiesUpdate {
//    std::optional<QString> name;
//    std::optional<QVector<int>> boardsOrdering;
//};

#endif // WORKSPACE_H
