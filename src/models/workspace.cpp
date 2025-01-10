#include "utilities/json_util.h"
#include "workspace.h"

QJsonObject Workspace::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"boardsOrdering", toJsonArray(boardsOrdering)},
        {"lastOpenedBoardId", lastOpenedBoardId}
    };
}

void Workspace::updateNodeProperties(const QJsonObject &obj) {
    if (const QJsonValue v = obj.value("name"); !v.isUndefined())
        name = v.toString();
    if (const QJsonValue v = obj.value("boardsOrdering"); !v.isUndefined())
        boardsOrdering = toIntVector(v.toArray(), -1);
}

void Workspace::updateNodeProperties(const WorkspaceNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(boardsOrdering);
    UPDATE_PROPERTY(lastOpenedBoardId);

#undef UPDATE_PROPERTY
}

QJsonObject WorkspaceNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (boardsOrdering.has_value())
        obj.insert("boardsOrdering", toJsonArray(boardsOrdering.value()));

    if (lastOpenedBoardId.has_value())
        obj.insert("lastOpenedBoardId", lastOpenedBoardId.value());

    return obj;
}
