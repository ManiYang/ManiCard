#include "utilities/json_util.h"
#include "workspaces_list_properties.h"

void WorkspacesListProperties::update(const QJsonObject &propertiesUpdate) {
    if (const QJsonValue v = propertiesUpdate.value("workspacesOrdering"); !v.isUndefined())
        workspacesOrdering = toIntVector(v.toArray(), -1);
}
