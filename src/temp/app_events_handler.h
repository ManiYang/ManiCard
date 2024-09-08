#ifndef APPEVENTSHANDLER_H
#define APPEVENTSHANDLER_H

#include <functional>
#include <QObject>
#include <QQueue>
#include "app_event_source.h"
#include "models/board.h"
#include "models/boards_list_properties.h"
#include "models/card.h"
#include "models/relationship.h"

class AppData;

class AppEventsHandler : public QObject
{
    Q_OBJECT
public:
    explicit AppEventsHandler(
            AppData *appData_, const QString &unsavedUpdateRecordFilePath_,
            QObject *parent = nullptr);

    // If data persistence fails, adds a record of unsaved update and shows a warning message box.

    void createdNewCard(
            const EventSource &eventSrc,
            const int cardId, const Card &card,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedCardProperties(
            const EventSource &eventSrc,
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedCardLabels(
            const EventSource &eventSrc,
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    //!
    //! The start/end cards must already exist (which is not checked here).
    //! It's not an error if the relationship already exists.
    //!
    void createdRelationship(
            const EventSource &eventSrc,
            const RelationshipId &id,
            std::function<void (bool ok, bool created)> callbackPersistResult,
            QPointer<QObject> callbackContext);

    void updatedUserRelationshipTypes(
            const EventSource &eventSrc,
            const QStringList &updatedRelTypes,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedUserCardLabels(
            const EventSource &eventSrc,
            const QStringList &updatedCardLabels,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedBoardsListProperties(
            const EventSource &eventSrc,
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void createdNewBoard(
            const EventSource &eventSrc,
            const int boardId, const Board &board,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedBoardNodeProperties(
            const EventSource &eventSrc,
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void removedBoard(
            const EventSource &eventSrc,
            const int boardId, std::function<void (bool ok)> callbackPersistResult,
            QPointer<QObject> callbackContext);

    void updatedNodeRectProperties(
            const EventSource &eventSrc,
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    //!
    //! The board and card must exist (must be saved beforehand).
    //!
    void createdNodeRect(
            const EventSource &eventSrc,
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void removedNodeRect(
            const EventSource &eventSrc,
            const int boardId, const int cardId,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedMainWindowSize(
            const EventSource &eventSrc,
            const QSize &size,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updatedHighlightedCardId(const EventSource &eventSrc, const int cardId);

signals:

private:
    const QString unsavedUpdateRecordFilePath;
    AppData *appData;

    QQueue<std::function<void ()>> taskQueue;
    bool taskInProgress {false};

    void addToQueue(std::function<void ()> func);
    void onTaskDone();
    void dequeueAndInvoke();

    // tools
    void showMsgOnUnsavedUpdate(const QString &dataName, const EventSource &eventSource);
};

#endif // APPEVENTSHANDLER_H
