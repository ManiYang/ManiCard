#include "utilities/json_util.h"
#include "workspaces_list_properties.h"

void WorkspacesListProperties::update(const QJsonObject &obj) {
    if (const QJsonValue v = obj.value("lastOpenedWorkspace"); !v.isUndefined())
        lastOpenedWorkspace = v.toInt(-1);

    if (const QJsonValue v = obj.value("workspacesOrdering"); !v.isUndefined())
        workspacesOrdering = toIntVector(v.toArray(), -1);
}

QJsonObject WorkspacesListPropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (lastOpenedWorkspace.has_value())
        obj.insert("lastOpenedWorkspace", lastOpenedWorkspace.value());

    if (workspacesOrdering.has_value())
        obj.insert("workspacesOrdering", toJsonArray(workspacesOrdering.value()));

    return obj;
}
