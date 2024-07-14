#include "boards_list_properties.h"
#include "utilities/json_util.h"

void BoardsListProperties::update(const BoardsListPropertiesUpdate &propertiesUpdate) {
#define UPDATE_PROPERTY(member) \
        if (propertiesUpdate.member.has_value()) \
            this->member = propertiesUpdate.member.value();

    UPDATE_PROPERTY(lastOpenedBoard);
    UPDATE_PROPERTY(boardsOrdering);

#undef UPDATE_PROPERTY
}

void BoardsListProperties::update(const QJsonObject &propertiesUpdate) {
    if (const auto v = propertiesUpdate.value("lastOpenedBoard"); !v.isUndefined())
        lastOpenedBoard = v.toInt();

    if (const auto v = propertiesUpdate.value("boardsOrdering"); !v.isUndefined())
        boardsOrdering = toIntVector(v.toArray(), -1);
}

//====

QJsonObject BoardsListPropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (lastOpenedBoard.has_value())
        obj.insert("lastOpenedBoard", lastOpenedBoard.value());

    if (boardsOrdering.has_value())
        obj.insert("boardsOrdering", toJsonArray(boardsOrdering.value()));

    return obj;
}

