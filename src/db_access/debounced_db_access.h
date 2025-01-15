#ifndef DEBOUNCEDDBACCESS_H
#define DEBOUNCEDDBACCESS_H

#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"
#include "app_event_source.h"

class ActionDebouncer;
class UnsavedUpdateRecordsFile;

//!
//! Provides access to DB, with a debounce mechanism for write operations, where contiguous calls
//! to certain write operation (for example, update of \e text property of the same card) can be
//! buffered (and accumulated) so that the frequency of actual DB write is limited.
//!
//! When a DB write operation fails, an unsaved-update record is added and a warning message box
//! is shown.
//!
//! Notes for implementing methods of this class:
//!
//! 1. Each write method is either debounced or not debounced. For a write method to be able to
//!    be debounced, its parameters (update data) must be able to be accumulated.
//!
//! 2. A debounced write-method, when called, creates a "debounce data key" using its parameters.
//!    It then starts a "debounce session" identified by that debounce data key, if a debounce
//!    session with the same debounce data key is not started yet.
//!
//! 3. When a debounced write-method is called within a debounce session with the same debounce
//!    data key, the update data (parameters of the operation) is accumulated and the actual DB
//!    operation is possibly delayed. When the operation is actually performed on DB, the
//!    accumulated update data is used (which is then cleared).
//!
//! 4. The debounce session is closed when one of the following is called:
//!     - a read method,
//!     - a write method that is not debounced,
//!     - a debounced write-method with different debounce data key (in this case a new debounce
//!       session is then started).
//!   Thus, there is at most one debounce session at a time. When a debounce session is closed,
//!   its delayed DB operation, if there is one, is performed immediately.
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

    //!
    //! Closes the debounce session, if there is one. May call a write-operation of
    //! \e cardsDataAccess (but does not wait for the operation to finish). Normally this
    //! should be called when the program is about to quit.
    //!
    void performPendingOperation();

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

    void queryCustomDataQueries(
            const QSet<int> &dataQueryIds,
            std::function<void (bool ok, const QHash<int, CustomDataQuery> &dataQueries)> callback,
            QPointer<QObject> callbackContext);

    void performCustomCypherQuery(
            const QString &cypher, const QJsonObject &parameters,
            std::function<void (bool, const QVector<QJsonObject> &)> callback,
            QPointer<QObject> callbackContext);

    // ==== cards data: write operations ====

    // If a write operation fails, a record of unsaved update is added and a warning message box
    // is shown.

    void createNewCardWithId(const int cardId, const Card &card);

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate); // debounced

    void updateCardLabels(const int cardId, const QSet<QString> &updatedLabels);

    void createRelationship(const RelationshipId &id);

    void updateUserRelationshipTypes(const QStringList &updatedRelTypes);

    void updateUserCardLabels(const QStringList &updatedCardLabels);

    void createNewCustomDataQueryWithId(
            const int customDataQueryId, const CustomDataQuery &customDataQuery);

    void updateCustomDataQueryProperties(
            const int customDataQueryId, const CustomDataQueryUpdate &update); // debounced

    // ==== boards data: read operations ====

    using WorkspaceIdAndData = std::pair<int, Workspace>;

    void getWorkspaces(
            std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
            QPointer<QObject> callbackContext);

    void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext);

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
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

    void createNewWorkspaceWithId(const int workspaceId, const Workspace &workspace);

    void updateWorkspaceNodeProperties(
            const int workspaceId, const WorkspaceNodePropertiesUpdate &update);

    void removeWorkspace(const int workspaceId);

    void updateWorkspacesListProperties(const WorkspacesListPropertiesUpdate &propertiesUpdate);

    void createNewBoardWithId(const int boardId, const Board &board, const int workspaceId);

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate);

    void removeBoard(const int boardId);

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update);

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData);

    void removeNodeRect(const int boardId, const int cardId);

    void createDataViewBox(
            const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData);

    void updateDataViewBoxProperties(
            const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update);

    void removeDataViewBox(const int boardId, const int customDataQueryId);

    void createTopLevelGroupBoxWithId(
            const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData);

    void updateGroupBoxProperties(const int groupBoxId, const GroupBoxNodePropertiesUpdate &update);

    void removeGroupBoxAndReparentChildItems(const int groupBoxId);

    void addOrReparentNodeRectToGroupBox(const int cardId, const int newGroupBoxId);

    void reparentGroupBox(const int groupBoxId, const int newParentGroupBox);

    void removeNodeRectFromGroupBox(const int cardId);

private:
    AbstractBoardsDataAccess *boardsDataAccess;
    AbstractCardsDataAccess *cardsDataAccess;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;

    //
    enum class DebounceDataCategory {
        CardProperties, CustomDataQueryProperties
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
        CustomDataQueryUpdate customDataQueryUpdate {};
    };
    CumulatedUpdateData cumulatedUpdateData;

    //
    void showMsgOnDbWriteFailed(const QString &dataName);
};

#endif // DEBOUNCEDDBACCESS_H
