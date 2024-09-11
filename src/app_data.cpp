#include "app_data.h"
#include "persisted_data_access.h"
#include "utilities/message_box.h"

AppData::AppData(PersistedDataAccess *persistedDataAccess_, QObject *parent)
        : AppDataReadonly(parent)
        , persistedDataAccess(persistedDataAccess_) {
}

void AppData::queryCards(
        const QSet<int> &cardIds, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryCards(cardIds, callback, callbackContext);
}

void AppData::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryRelationship(relationshipId, callback, callbackContext);
}

void AppData::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryRelationshipsFromToCards(cardIds, callback, callbackContext);
}

void AppData::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getUserLabelsAndRelationshipTypes(callback, callbackContext);
}

void AppData::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    persistedDataAccess->requestNewCardId(callback, callbackContext);
}

void AppData::getBoardIdsAndNames(
        std::function<void (bool, const QHash<int, QString> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getBoardIdsAndNames(callback, callbackContext);
}

void AppData::getBoardsListProperties(
        std::function<void (bool, BoardsListProperties)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getBoardsListProperties(callback, callbackContext);
}

void AppData::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getBoardData(boardId, callback, callbackContext);
}

void AppData::requestNewBoardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    persistedDataAccess->requestNewBoardId(callback, callbackContext);
}

std::optional<QSize> AppData::getMainWindowSize() {
    return persistedDataAccess->getMainWindowSize();
}

void AppData::createNewCardWithId(
        const EventSource &/*eventSrc*/, const int cardId, const Card &card) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewCardWithId(cardId, card);
}

void AppData::updateCardProperties(
        const EventSource &eventSrc, const int cardId,
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    // 1. synchornously update all variables and emit "updated" signals
    emit cardPropertiesUpdated(eventSrc, cardId, cardPropertiesUpdate);

    // 2. persist
    persistedDataAccess->updateCardProperties(cardId, cardPropertiesUpdate);
}

void AppData::updateCardLabels(
        const EventSource &/*eventSrc*/, const int cardId, const QSet<QString> &updatedLabels) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateCardLabels(cardId, updatedLabels);
}

void AppData::createRelationship(const EventSource &/*eventSrc*/, const RelationshipId &id) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createRelationship(id);
}

void AppData::updateUserRelationshipTypes(
        const EventSource &/*eventSrc*/, const QStringList &updatedRelTypes)  {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateUserRelationshipTypes(updatedRelTypes);
}

void AppData::updateUserCardLabels(
        const EventSource &/*eventSrc*/, const QStringList &updatedCardLabels) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateUserCardLabels(updatedCardLabels);
}

void AppData::updateBoardsListProperties(
        const EventSource &/*eventSrc*/, const BoardsListPropertiesUpdate &propertiesUpdate) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardsListProperties(propertiesUpdate);
}

void AppData::createNewBoardWithId(
        const EventSource &/*eventSrc*/, const int boardId, const Board &board) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewBoardWithId(boardId, board);
}

void AppData::updateBoardNodeProperties(
        const EventSource &/*eventSrc*/, const int boardId,
        const BoardNodePropertiesUpdate &propertiesUpdate) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardNodeProperties(boardId, propertiesUpdate);
}

void AppData::removeBoard(const EventSource &/*eventSrc*/, const int boardId) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeBoard(boardId);
}

void AppData::updateNodeRectProperties(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectDataUpdate &update) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateNodeRectProperties(boardId, cardId, update);
}

void AppData::createNodeRect(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectData &nodeRectData) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNodeRect(boardId, cardId, nodeRectData);
}

void AppData::removeNodeRect(
        const EventSource &/*eventSrc*/, const int boardId, const int cardId) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeNodeRect(boardId, cardId);
}

void AppData::updateMainWindowSize(const EventSource &/*eventSrc*/, const QSize &size) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->saveMainWindowSize(size);
}

int AppData::getHighlightedCardId() const {
    return highlightedCardId;
}

void AppData::setHighlightedCardId(const EventSource &eventSrc, const int cardId) {
    // synchornously update all variables and emit "updated" signals
    highlightedCardId = cardId;
    emit highlightedCardIdUpdated(eventSrc);
}
