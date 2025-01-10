#ifndef WORKSPACES_LIST_PROPERTIES_H
#define WORKSPACES_LIST_PROPERTIES_H

#include <QJsonObject>
#include <QVector>

struct WorkspacesListProperties
{
    int lastOpenedWorkspace {-1};
    QVector<int> workspacesOrdering; // may not contain all existing workspace IDs, and may contain
                                     // non-existing workspace IDs

    //
    void update(const QJsonObject &propertiesUpdate);
};


struct WorkspacesListPropertiesUpdate
{
    std::optional<int> lastOpenedWorkspace;
    std::optional<QVector<int>> workspacesOrdering;

    QJsonObject toJson() const;
};

#endif // WORKSPACES_LIST_PROPERTIES_H
