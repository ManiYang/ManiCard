#include "app_data.h"
#include "persisted_data_access.h"

AppData::AppData(PersistedDataAccess *persistedDataAccess_, QObject *parent)
        : QObject(parent)
        , persistedDataAccess(persistedDataAccess_) {
}

void AppData::queryCards(
        const QSet<int> &cardIds, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryCards(cardIds, callback, callbackContext);
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
        const int cardId, const Card &card,
        std::function<void (bool)> callbackPersistResult,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewCardWithId(cardId, card, callbackPersistResult, callbackContext);
}

void AppData::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateCardProperties(
                cardId, cardPropertiesUpdate, callback, callbackContext);
}

void AppData::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateCardLabels(cardId, updatedLabels, callback, callbackContext);
}

void AppData::createRelationship(
        const RelationshipId &id, std::function<void (bool, bool)> callback,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createRelationship(id, callback, callbackContext);
}

void AppData::updateUserRelationshipTypes(
        const QStringList &updatedRelTypes, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext)  {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist{
    persistedDataAccess->updateUserRelationshipTypes(
                updatedRelTypes, callback, callbackContext);
}

void AppData::updateUserCardLabels(
        const QStringList &updatedCardLabels, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateUserCardLabels(updatedCardLabels, callback, callbackContext);
}

void AppData::updateBoardsListProperties(
        const BoardsListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardsListProperties(propertiesUpdate, callback, callbackContext);
}

void AppData::createNewBoardWithId(
        const int boardId, const Board &board, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNewBoardWithId(boardId, board, callback, callbackContext);
}

void AppData::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateBoardNodeProperties(
                boardId, propertiesUpdate, callback, callbackContext);
}

void AppData::removeBoard(
        const int boardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeBoard(boardId, callback, callbackContext);
}

void AppData::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->updateNodeRectProperties(
                boardId, cardId, update, callback, callbackContext);
}

void AppData::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->createNodeRect(boardId, cardId, nodeRectData, callback, callbackContext);
}

void AppData::removeNodeRect(
        const int boardId, const int cardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    persistedDataAccess->removeNodeRect(boardId, cardId, callback, callbackContext);
}

bool AppData::saveMainWindowSize(const QSize &size) {
    // 1. synchornously update all variables and emit "updated" signals

    // 2. persist
    return persistedDataAccess->saveMainWindowSize(size);
}

int AppData::getHighlightedCardId() const {
    return highlightedCardId;
}

void AppData::setHighlightedCardId(const int cardId, EventSource eventSrc) {
    // synchornously update all variables and emit "updated" signals
    highlightedCardId = cardId;
    emit highlightedCardIdUpdated(eventSrc);
}
