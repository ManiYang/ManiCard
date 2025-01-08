#ifndef WORKSPACES_LIST_PROPERTIES_H
#define WORKSPACES_LIST_PROPERTIES_H

#include <QJsonObject>
#include <QVector>

struct WorkspacesListProperties {
    QVector<int> workspacesOrdering; // may not contain all existing workspace IDs, and may contain
                                     // non-existing workspace IDs

    //
    void update(const QJsonObject &propertiesUpdate);
};

#endif // WORKSPACES_LIST_PROPERTIES_H
