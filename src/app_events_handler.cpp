#include <QTimer>
#include "app_data.h"
#include "app_events_handler.h"
#include "utilities/functor.h"

AppEventsHandler::AppEventsHandler(AppData *appData_, QObject *parent)
    : QObject(parent)
    , appData(appData_) {
}

void AppEventsHandler::createdNewCard(
        const EventSource &eventSrc, const int cardId, const Card &card,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->createNewCardWithId(
                eventSrc, cardId, card, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedCardProperties(
        const EventSource &eventSrc, const int cardId,
        const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateCardProperties(
                eventSrc, cardId, cardPropertiesUpdate, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedCardLabels(
        const EventSource &eventSrc, const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateCardLabels(
                eventSrc, cardId, updatedLabels, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::createdRelationship(
        const EventSource &eventSrc, const RelationshipId &id,
        std::function<void (bool, bool)> callbackPersistResult,
        QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->createRelationship(eventSrc, id, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedUserRelationshipTypes(
        const EventSource &eventSrc, const QStringList &updatedRelTypes,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateUserRelationshipTypes(
                eventSrc, updatedRelTypes, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedUserCardLabels(
        const EventSource &eventSrc, const QStringList &updatedCardLabels,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateUserCardLabels(
                eventSrc, updatedCardLabels, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedBoardsListProperties(
        const EventSource &eventSrc, const BoardsListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateBoardsListProperties(
                eventSrc, propertiesUpdate, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::createdNewBoard(
        const EventSource &eventSrc, const int boardId, const Board &board,
        std::function<void (bool)> callbackPersistResult,
        QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->createNewBoardWithId(
                eventSrc, boardId, board, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedBoardNodeProperties(
        const EventSource &eventSrc, const int boardId,
        const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateBoardNodeProperties(
                eventSrc, boardId, propertiesUpdate, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::removedBoard(
        const EventSource &eventSrc, const int boardId,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->removeBoard(eventSrc, boardId, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedNodeRectProperties(
        const EventSource &eventSrc, const int boardId, const int cardId,
        const NodeRectDataUpdate &update,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->updateNodeRectProperties(
                eventSrc, boardId, cardId, update, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::createdNodeRect(
        const EventSource &eventSrc, const int boardId, const int cardId,
        const NodeRectData &nodeRectData,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->createNodeRect(
                eventSrc, boardId, cardId, nodeRectData, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::removedNodeRect(
        const EventSource &eventSrc, const int boardId, const int cardId,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        appData->removeNodeRect(
                eventSrc, boardId, cardId, callbackPersistResult, callbackContext);
    });
}

void AppEventsHandler::updatedMainWindowSize(
        const EventSource &eventSrc, const QSize &size,
        std::function<void (bool)> callbackPersistResult, QPointer<QObject> callbackContext) {
    addToQueue([=]() {
        const bool ok = appData->updateMainWindowSize(eventSrc, size);
        invokeAction(callbackContext, [callbackPersistResult, ok]() {
            callbackPersistResult(ok);
        });
    });
}

void AppEventsHandler::updatedHighlightedCardId(const EventSource &eventSrc, const int cardId) {
    addToQueue([=]() {
        appData->setHighlightedCardId(eventSrc, cardId);
    });
}

void AppEventsHandler::addToQueue(std::function<void ()> func) {
    taskQueue << func;

    if (!taskInProgress) {
        dequeueAndInvoke();
        taskInProgress = true;
    }
}

void AppEventsHandler::onTaskDone() {
    if (!taskQueue.isEmpty())
        dequeueAndInvoke();
    else
        taskInProgress = false;
}

void AppEventsHandler::dequeueAndInvoke() {
    Q_ASSERT(!taskQueue.isEmpty());
    auto task = taskQueue.dequeue();

    // add `func` to the Qt event queue (rather than call it directly) to prevent deep call stack
    QTimer::singleShot(0, this, [task]() { task(); });
}
