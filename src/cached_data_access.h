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

//!
//! Accesses DB & files, and manages cached data.
//!
//! For read operation, this class
//!   1. gets the parts that are already cached
//!   2. reads from DB or files for the other parts, if any, and if successful, updates the cache.
//! The operation fails only if step 2 is needed and fails.
//!
//! For write operation, this class
//!   1. updates the cache
//!   2. writes to DB or files, and if failed, adds to the records of unsaved updates.
//!
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

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext);

    using StringListPair = std::pair<QStringList, QStringList>;
    void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
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

    std::optional<QSize> getMainWindowSize();

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
    //! It's not an error if the relationship already exists.
    //!
    void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext);

    void updateUserRelationshipTypes(
            const QStringList &updatedRelTypes, std::function<void (bool ok)> callback,
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

    bool saveMainWindowSize(const QSize &size);

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

        std::optional<QStringList> userLabelsList;
        std::optional<QStringList> userRelTypesList;
    };
    Cache cache;

    //
    int lastWriteRequestId {-1};
    QSet<int> writeRequestsInProgress;
    mutable QReadWriteLock lockForwriteRequestsInProgress;

    int startWriteRequest();
    void finishWriteRequest(const int requestId); // thread-safe
};

#endif // CACHEDDATAACCESS_H
