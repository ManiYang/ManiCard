#ifndef QUEUED_DB_ACCESS_H
#define QUEUED_DB_ACCESS_H

#include <functional>
#include <memory>
#include <type_traits>
#include <QObject>
#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"

//!
//! A proxy of \c BoardsDataAccess & \c CardsDataAccess. The requests are queued and handled in
//! sequence (sends next request only after current one gets response).
//!
//! When a non-read-only operation failed, all remaining requests in the queue will fail directly
//! (without actually being performed). Before the error flag is cleared, any new request will also
//! fail directly.
//!
class QueuedDbAccess
        : public QObject, public AbstractBoardsDataAccess, public AbstractCardsDataAccess
{
    Q_OBJECT
public:
    QueuedDbAccess(
            std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
            std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess,
            QObject *parent = nullptr);

    void clearErrorFlag();
    bool hasUnfinishedOperation() const;

    // ==== AbstractCardsDataAccess interface ====

    // read operations

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void traverseFromCard(
            const int startCardId,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationship(
            const RelId &relationshipId,
            std::function<void (bool ok, const std::optional<RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

    void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext) override;

    void queryCustomDataQueries(
            const QSet<int> &dataQueryIds,
            std::function<void (bool ok, const QHash<int, CustomDataQuery> &dataQueries)> callback,
            QPointer<QObject> callbackContext) override;

    void performCustomCypherQuery(
            const QString &cypher, const QJsonObject &parameters,
            std::function<void (bool, const QVector<QJsonObject> &)> callback,
            QPointer<QObject> callbackContext) override;

    void requestNewCardId(
            std::function<void (bool ok, int cardId)> callback,
            QPointer<QObject> callbackContext) override;

    // write operations

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext) override;

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext) override;

    void updateUserRelationshipTypes(
            const QStringList &updatedRelTypes, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext) override;

    void updateUserCardLabels(
            const QStringList &updatedCardLabels, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext) override;

    void createNewCustomDataQueryWithId(
            const int customDataQueryId, const CustomDataQuery &customDataQuery,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext) override;

    void updateCustomDataQueryProperties(
            const int customDataQueryId, const CustomDataQueryUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    // ==== AbstractBoardsDataAccess interface ====

    // read operations

    void getWorkspaces(
            std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
            QPointer<QObject> callbackContext) override;

    void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
                const int boardId,
                std::function<void (bool ok, std::optional<Board> board)> callback,
                QPointer<QObject> callbackContext) override;

    // write operations

    void createNewWorkspaceWithId(
            const int workspaceId, const Workspace &workspace,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateWorkspaceNodeProperties(
            const int workspaceId, const WorkspaceNodePropertiesUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeWorkspace(
            const int workspaceId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateWorkspacesListProperties(
            const WorkspacesListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void requestNewBoardId(
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext) override;

    void createNewBoardWithId(
            const int boardId, const Board &board, const int workspaceId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeBoard(
            const int boardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeNodeRect(
            const int boardId, const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void createDataViewBox(
                const int boardId, const int customDataQueryId,
                const DataViewBoxData &dataViewBoxData,
                std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateDataViewBoxProperties(
            const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeDataViewBox(
            const int boardId, const int customDataQueryId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void createTopLevelGroupBoxWithId(
            const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateGroupBoxProperties(
            const int groupBoxId, const GroupBoxDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeGroupBoxAndReparentChildItems(
            const int groupBoxId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

private:
    std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess;
    std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess;

    struct Task
    {
        std::function<void (const bool failDirectly)> func;
        bool toFailDirectly;
    };
    QQueue<Task> queue;

    bool isWaitingResponse {false};
    bool errorFlag {false}; // set when a request failed, unset by clearErrorFlag()

    void addToQueue(std::function<void (const bool failDirectly)> func);
    void onResponse(const bool ok, const bool isReadOnlyAccess = false);
    void dequeueAndInvoke();

    //
    struct Void {};

    template <typename Result, typename... InputArgs>
    struct FunctionTypeHelper
    {
        using Callback = std::function<void (bool ok, Result result)>;
        using Func = std::function<void (InputArgs..., Callback, QPointer<QObject>)>;
        static constexpr bool hasResultArg {true};
    };

    template <typename... InputArgs>
    struct FunctionTypeHelper<Void, InputArgs...>
    {
        using Callback = std::function<void (bool ok)>;
        using Func = std::function<void (InputArgs..., Callback, QPointer<QObject>)>;
        static constexpr bool hasResultArg {false};
    };

    //!
    //! Type \e Result can be \c Void, meaning the callback function has only the \c bool argument.
    //! Type \e Result, if is not \c Void, must have default constructor.
    //!
    template <bool isReadOnly, typename Result, typename... InputArgs>
    std::function<void (const bool failDirectly)> createTask(
            typename FunctionTypeHelper<Result, InputArgs...>::Func func,
            InputArgs... inputValues,
            typename FunctionTypeHelper<Result, InputArgs...>::Callback callback,
            QPointer<QObject> callbackContext
    ) {
        return [=, thisPtr=QPointer(this)](const bool failDirectly) {
            if (failDirectly) {
                if constexpr (FunctionTypeHelper<Result, InputArgs...>::hasResultArg) {
                    invokeAction(callbackContext, [callback]() {
                        callback(false, Result {});
                    });
                }
                else {
                    invokeAction(callbackContext, [callback]() {
                        callback(false);
                    });
                }

                if (thisPtr)
                    thisPtr->onResponse(false, isReadOnly);
                return;
            }

            if (thisPtr.isNull())
                return;

            func(
                    inputValues...,
                    // callback:
                    [thisPtr, callback, callbackContext](bool ok, auto... rest) {
                        invokeAction(callbackContext, [=]() {
                            callback(ok, rest...);
                        });
                        if (thisPtr)
                            thisPtr->onResponse(ok, isReadOnly);
                    },
                    thisPtr.data()
            );
        };
    };
};

#endif // QUEUED_DB_ACCESS_H
