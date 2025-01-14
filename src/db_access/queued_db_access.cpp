#include <QTimer>
#include "queued_db_access.h"
#include "utilities/functor.h"

QueuedDbAccess::QueuedDbAccess(
        std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
        std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess, QObject *parent)
    : QObject(parent)
    , boardsDataAccess(boardsDataAccess)
    , cardsDataAccess(cardsDataAccess)
{}

void QueuedDbAccess::clearErrorFlag() {
    errorFlag = false;
}

bool QueuedDbAccess::hasUnfinishedOperation() const {
    return isWaitingResponse;
}

void QueuedDbAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<int, Card> & // result type (`Void` if no result argument)
                    , decltype(cardIds) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->queryCards(args...); // method
            },
            cardIds, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::traverseFromCard(
        const int startCardId, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<int, Card> & // result type (`Void` if no result argument)
                    , decltype(startCardId) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->traverseFromCard(args...); // method
            },
            startCardId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const std::optional<RelProperties> & // result type (`Void` if no result argument)
                    , decltype(relationshipId) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->queryRelationship(args...); // method
            },
            relationshipId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<RelId, RelProperties> & // result type (`Void` if no result argument)
                    , decltype(cardIds) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->queryRelationshipsFromToCards(args...); // method
            },
            cardIds, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const StringListPair & // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                cardsDataAccess->getUserLabelsAndRelationshipTypes(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::queryCustomDataQueries(
        const QSet<int> &dataQueryIds,
        std::function<void (bool, const QHash<int, CustomDataQuery> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<int, CustomDataQuery> &, // result type (`Void` if no result argument)
                    decltype(dataQueryIds) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->queryCustomDataQueries(args...); // method
            },
            dataQueryIds, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::performCustomCypherQuery(
        const QString &cypher, const QJsonObject &parameters,
        std::function<void (bool, const QVector<QJsonObject> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QVector<QJsonObject> & // result type (`Void` if no result argument)
                    , decltype(cypher), decltype(parameters) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->performCustomCypherQuery(args...); // method
            },
            cypher, parameters, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::requestNewCardId(
        std::function<void (bool, int)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , int // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                cardsDataAccess->requestNewCardId(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createNewCardWithId(
        const int cardId, const Card &card,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(cardId), decltype(card) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->createNewCardWithId(args...); // method
            },
            cardId, card, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(cardId), decltype(cardPropertiesUpdate) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->updateCardProperties(args...); // method
            },
            cardId, cardPropertiesUpdate, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(cardId), decltype(updatedLabels) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->updateCardLabels(args...); // method
            },
            cardId, updatedLabels, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createRelationship(
        const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , bool // result type (`Void` if no result argument)
                    , decltype(id) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->createRelationship(args...); // method
            },
            id, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateUserRelationshipTypes(
        const QStringList &updatedRelTypes, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(updatedRelTypes) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->updateUserRelationshipTypes(args...); // method
            },
            updatedRelTypes, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateUserCardLabels(
        const QStringList &updatedCardLabels, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(updatedCardLabels) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->updateUserCardLabels(args...); // method
            },
            updatedCardLabels, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createNewCustomDataQueryWithId(
        const int customDataQueryId, const CustomDataQuery &customDataQuery,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(customDataQueryId), decltype(customDataQuery) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->createNewCustomDataQueryWithId(args...); // method
            },
            customDataQueryId, customDataQuery, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateCustomDataQueryProperties(
        const int customDataQueryId, const CustomDataQueryUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(customDataQueryId), decltype(update) // input types
                >(
            [this](auto... args) {
                cardsDataAccess->updateCustomDataQueryProperties(args...); // method
            },
            customDataQueryId, update, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::getWorkspaces(
        std::function<void (bool, const QHash<int, Workspace> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<int, Workspace> & // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                boardsDataAccess->getWorkspaces(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::getWorkspacesListProperties(
        std::function<void (bool, WorkspacesListProperties)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , WorkspacesListProperties // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                boardsDataAccess->getWorkspacesListProperties(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::getBoardIdsAndNames(
        std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , const QHash<int, QString> & // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                boardsDataAccess->getBoardIdsAndNames(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , std::optional<Board> // result type (`Void` if no result argument)
                    , decltype(boardId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->getBoardData(args...); // method
            },
            boardId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createNewWorkspaceWithId(
        const int workspaceId, const Workspace &workspace,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    Q_ASSERT(workspace.boardIds.isEmpty()); // new workspace should have no board

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(workspaceId), decltype(workspace) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->createNewWorkspaceWithId(args...); // method
            },
            workspaceId, workspace, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateWorkspaceNodeProperties(
        const int workspaceId, const WorkspaceNodePropertiesUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(workspaceId), decltype(update) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateWorkspaceNodeProperties(args...); // method
            },
            workspaceId, update, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::removeWorkspace(
        const int workspaceId, std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(workspaceId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->removeWorkspace(args...); // method
            },
            workspaceId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateWorkspacesListProperties(
        const WorkspacesListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(propertiesUpdate) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateWorkspacesListProperties(args...); // method
            },
            propertiesUpdate, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::requestNewBoardId(
        std::function<void (bool, int)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    true // is readonly?
                    , int // result type (`Void` if no result argument)
                    // no input // input types
                >(
            [this](auto... args) {
                boardsDataAccess->requestNewBoardId(args...); // method
            },
            // no input // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createNewBoardWithId(
        const int boardId, const Board &board, const int workspaceId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    Q_ASSERT(board.cardIdToNodeRectData.isEmpty()); // new board should have no NodeRect

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(board), decltype(workspaceId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->createNewBoardWithId(args...); // method
            },
            boardId, board, workspaceId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(propertiesUpdate) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateBoardNodeProperties(args...); // method
            },
            boardId, propertiesUpdate, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::removeBoard(
        const int boardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->removeBoard(args...); // method
            },
            boardId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(cardId), decltype(update) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateNodeRectProperties(args...); // method
            },
            boardId, cardId, update, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(cardId), decltype(nodeRectData) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->createNodeRect(args...); // method
            },
            boardId, cardId, nodeRectData, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::removeNodeRect(
        const int boardId, const int cardId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(cardId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->removeNodeRect(args...); // method
            },
            boardId, cardId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createDataViewBox(
        const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId)
                    , decltype(customDataQueryId), decltype(dataViewBoxData) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->createDataViewBox(args...); // method
            },
            boardId, customDataQueryId, dataViewBoxData, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateDataViewBoxProperties(
        const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId)
                    , decltype(customDataQueryId), decltype(update) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateDataViewBoxProperties(args...); // method
            },
            boardId, customDataQueryId, update, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::removeDataViewBox(
        const int boardId, const int customDataQueryId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(customDataQueryId) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->removeDataViewBox(args...); // method
            },
            boardId, customDataQueryId, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::createTopLevelGroupBoxWithId(
        const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(boardId), decltype(groupBoxId), decltype(groupBoxData) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->createTopLevelGroupBoxWithId(args...); // method
            },
            boardId, groupBoxId, groupBoxData, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::updateGroupBoxProperties(
        const int groupBoxId, const GroupBoxDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    auto func = createTask<
                    false // is readonly?
                    , Void // result type (`Void` if no result argument)
                    , decltype(groupBoxId), decltype(update) // input types
                >(
            [this](auto... args) {
                boardsDataAccess->updateGroupBoxProperties(args...); // method
            },
            groupBoxId, update, // input parameters
            callback, callbackContext
    );

    addToQueue(func);
}

void QueuedDbAccess::addToQueue(std::function<void (const bool)> func) {
    const bool failDirectly = errorFlag;
    queue << Task {func, failDirectly};

    if (!isWaitingResponse) {
        dequeueAndInvoke();
        isWaitingResponse = true;
    }
}

void QueuedDbAccess::onResponse(const bool ok, const bool isReadOnlyAccess) {
    if (!isReadOnlyAccess && !ok) {
        if (!errorFlag) {
            errorFlag = true;
            // let all remaining task fail directly (without data access)
            for (Task &task: queue)
                task.toFailDirectly = true;
        }
    }

    if (!queue.isEmpty())
        dequeueAndInvoke();
    else
        isWaitingResponse = false;
}

void QueuedDbAccess::dequeueAndInvoke() {
    Q_ASSERT(!queue.isEmpty());
    auto task = queue.dequeue();

    // add `func` to the event queue (rather than call it directly) to prevent deep call stack
    QTimer::singleShot(0, this, [task]() {
        task.func(task.toFailDirectly);
    });
}
