#include "app_data.h"
#include "persisted_data_access.h"

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
        const EventSource &/*eventSrc*/, const int cardId, const Card &card,
        std::function<void (bool)> callbackPersistResult,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewCardWithId(cardId, card, callbackPersistResult, callbackContext);
}

void AppData::updateCardProperties(
        const EventSource &eventSrc,
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals
    emit cardPropertiesUpdated(eventSrc, cardId, cardPropertiesUpdate);

    // 2. persist
    persistedDataAccess->updateCardProperties(
                cardId, cardPropertiesUpdate, callbackPersistResult, callbackContext);
}

void AppData::updateCardLabels(
        const EventSource &/*eventSrc*/, const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateCardLabels(cardId, updatedLabels, callbackPersistResult, callbackContext);
}

void AppData::createRelationship(
        const EventSource &/*eventSrc*/, const RelationshipId &id,
        std::function<void (bool, bool)> callbackPersistResult,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createRelationship(id, callbackPersistResult, callbackContext);
}

void AppData::updateUserRelationshipTypes(
        const EventSource &/*eventSrc*/, const QStringList &updatedRelTypes,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext)  {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist{
    persistedDataAccess->updateUserRelationshipTypes(
                updatedRelTypes, callbackPersistResult, callbackContext);
}

void AppData::updateUserCardLabels(
        const EventSource &/*eventSrc*/, const QStringList &updatedCardLabels,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateUserCardLabels(
            updatedCardLabels, callbackPersistResult, callbackContext);
}

void AppData::updateBoardsListProperties(
        const EventSource &/*eventSrc*/, const BoardsListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardsListProperties(
            propertiesUpdate, callbackPersistResult, callbackContext);
}

void AppData::createNewBoardWithId(
        const EventSource &/*eventSrc*/, const int boardId, const Board &board,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewBoardWithId(
            boardId, board, callbackPersistResult, callbackContext);
}

void AppData::updateBoardNodeProperties(
        const EventSource &/*eventSrc*/, const int boardId,
        const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardNodeProperties(
                boardId, propertiesUpdate, callbackPersistResult, callbackContext);
}

void AppData::removeBoard(
        const EventSource &/*eventSrc*/, const int boardId,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeBoard(boardId, callbackPersistResult, callbackContext);
}

void AppData::updateNodeRectProperties(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectDataUpdate &update,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateNodeRectProperties(
                boardId, cardId, update, callbackPersistResult, callbackContext);
}

void AppData::createNodeRect(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectData &nodeRectData,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNodeRect(
            boardId, cardId, nodeRectData, callbackPersistResult, callbackContext);
}

void AppData::removeNodeRect(
        const EventSource &/*eventSrc*/, const int boardId, const int cardId,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeNodeRect(boardId, cardId, callbackPersistResult, callbackContext);
}

bool AppData::updateMainWindowSize(const EventSource &/*eventSrc*/, const QSize &size) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    return persistedDataAccess->saveMainWindowSize(size);
}

int AppData::getHighlightedCardId() const {
    return highlightedCardId;
}

void AppData::setHighlightedCardId(const EventSource &eventSrc, const int cardId) {
    // synchornously update all variables and emit "updated" signals
    highlightedCardId = cardId;
    emit highlightedCardIdUpdated(eventSrc);
}
