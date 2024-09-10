#ifndef DEBOUNCEDDBACCESS_H
#define DEBOUNCEDDBACCESS_H

#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"

class ActionDebouncer;

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
            std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
            std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess,
            QObject *parent = nullptr);

    void finalize();

    // ==== AbstractCardsDataAccess interface ====

    // read operations

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

    // write operations

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext);

    void updateCardPropertiesDebounced(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext);

    void updateUserRelationshipTypes(
            const QStringList &updatedRelTypes, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext);

    void updateUserCardLabels(
            const QStringList &updatedCardLabels, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext);

    // ==== AbstractBoardsDataAccess interface ====

    // read operations

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

    // write operations

    void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void requestNewBoardId(
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext);

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
    std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess;
    std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess;

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
};

#endif // DEBOUNCEDDBACCESS_H
