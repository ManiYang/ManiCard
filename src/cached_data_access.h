#ifndef CACHEDDATAACCESS_H
#define CACHEDDATAACCESS_H

#include <QObject>
#include <QPointer>
#include <QReadWriteLock>
#include "models/board.h"
#include "models/boards_list_properties.h"
#include "models/card.h"

class LocalSettingsFile;
class QueuedDbAccess;
class UnsavedUpdateRecordsFile;

class CachedDataAccess : public QObject
{
    Q_OBJECT
public:
    explicit CachedDataAccess(
            QueuedDbAccess *queuedDbAccess_,
            std::shared_ptr<LocalSettingsFile> localSettingsFile_,
            std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
            QObject *parent = nullptr);

    bool hasWriteRequestInProgress() const;

    // ==== read ====

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext);

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext);

    void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext);

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext);

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext);

    // ==== write ====

    // A write operation fails if data cannot be saved to DB or file. In this case, a record of
    // unsaved update is added.

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    //!
    //! The start/end cards must already exist (which is not checked here). Otherwise the cache
    //! will become inconsistent with DB.
    //!
    void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext);

    void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void createNewBoardWithId(
            const int boardId, const Board &board,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void removeBoard(
            const int boardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void removeNodeRect(
            const int boardId, const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

private:
    QueuedDbAccess *queuedDbAccess;
    std::shared_ptr<LocalSettingsFile> localSettingsFile;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;

    // data cache
    struct Cache
    {
        QHash<int, Board> boards;
        QHash<int, Card> cards;
        QHash<RelationshipId, RelationshipProperties> relationships;
    };
    Cache cache;

    //
    int lastWriteRequestId {-1};
    QSet<int> writeRequestsInProgress;
    mutable QReadWriteLock lockForwriteRequestsInProgress;

    int startWriteRequest();
    void finishWriteRequest(const int requestId); // thread-safe
};

/*
 * Dev notes:
 *
 * + For read operation:
 *   1. get the parts that are already cached
 *   2. query DB & local settings file for the other parts (if any)
 *      - if successful: update cache
 *      - if failed: whole process fails
 * + For write operation:
 *   1. update cache
 *   2. Write DB. If failed, add to unsaved updates.
 *   3. Write local settings file. If failed, add to unsaved updates.
*/

#endif // CACHEDDATAACCESS_H
