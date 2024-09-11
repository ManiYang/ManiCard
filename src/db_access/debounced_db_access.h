#ifndef DEBOUNCEDDBACCESS_H
#define DEBOUNCEDDBACCESS_H

#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"
#include "app_event_source.h"

class ActionDebouncer;
class UnsavedUpdateRecordsFile;

//!
//! A debounced write-operation may start a "debounce session" with a "debounce data key".
//! The debounce session is closed when one of the following is invoked:
//!   - a read operation,
//!   - a write operation that is not debounced,
//!   - a debounced write-operation with different debounce data key. (In this case a
//!     new debounce session is started.)
//! If a write operation is invoked within a debounce session with the same debounce data key,
//! the update data (parameters of the operation) is accumulated and the operation is
//! debounced.
//!
class DebouncedDbAccess : public QObject
{
    Q_OBJECT
public:
    DebouncedDbAccess(
            AbstractBoardsDataAccess *boardsDataAccess_,
            AbstractCardsDataAccess *cardsDataAccess_,
            std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
            QObject *parent = nullptr);

    void finalize();

    // ==== cards data: read operations ====

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    void traverseFromCard(
            const int startCardId,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    void queryRelationship(
            const RelId &relationshipId,
            std::function<void (bool ok, const std::optional<RelProperties> &)> callback,
            QPointer<QObject> callbackContext);

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext);

    using StringListPair = std::pair<QStringList, QStringList>;

    void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext);

    void requestNewCardId(
            std::function<void (bool ok, int cardId)> callback,
            QPointer<QObject> callbackContext);

    // ==== cards data: write operations ====

    // If a write operation fails, a record of unsaved update is added and a warning message box
    // is shown.

    void createNewCardWithId(const int cardId, const Card &card);

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);
            // debounced

    void updateCardLabels(const int cardId, const QSet<QString> &updatedLabels);

    void createRelationship(const RelationshipId &id);

    void updateUserRelationshipTypes(const QStringList &updatedRelTypes);

    void updateUserCardLabels(const QStringList &updatedCardLabels);

    // ==== boards data: read operations ====

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
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext);

    // ==== boards data: write operations ====

    // If a write operation fails, a record of unsaved update is added and a warning message box
    // is shown.

    void updateBoardsListProperties(const BoardsListPropertiesUpdate &propertiesUpdate);

    void createNewBoardWithId(const int boardId, const Board &board);

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate);

    void removeBoard(const int boardId);

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update);

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData);

    void removeNodeRect(const int boardId, const int cardId);

private:
    AbstractBoardsDataAccess *boardsDataAccess;
    AbstractCardsDataAccess *cardsDataAccess;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;

    //
    enum class DebounceDataCategory {
        CardProperties
    };
    static QString debounceDataCategoryName(const DebounceDataCategory category);

    using DebounceKey = std::pair<DebounceDataCategory, QJsonObject>;

    struct DebounceSession
    {
        explicit DebounceSession(
                const DebounceKey &debounceKey, const int separationMsec,
                std::function<void ()> action);
        DebounceSession(const DebounceSession &other) = delete;
        DebounceSession &operator =(const DebounceSession &other) = delete;

        //!
        //! Closes the session, possibly invokes the action.
        //!
        ~DebounceSession();

        DebounceDataCategory category;
        QJsonObject dataKeys;

        DebounceKey key() const;
        void tryAct();

        QString printKey() const;

    private:
        ActionDebouncer *debouncer;
    };
    std::optional<DebounceSession> currentDebounceSession;

    void closeDebounceSession();

    //
    struct CumulatedUpdateData
    {
        CardPropertiesUpdate cardPropertiesUpdate {};
    };
    CumulatedUpdateData cumulatedUpdateData;

    //
    void showMsgOnDbWriteFailed(const QString &dataName);
};

#endif // DEBOUNCEDDBACCESS_H
