#include "utilities/json_util.h"
#include "workspace.h"

void Workspace::updateNodeProperties(const QJsonObject &obj) {
    if (const QJsonValue v = obj.value("name"); !v.isUndefined())
        name = v.toString();
    if (const QJsonValue v = obj.value("boardsOrdering"); !v.isUndefined())
        boardsOrdering = toIntVector(v.toArray(), -1);
}
