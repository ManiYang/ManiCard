#include "boards_data_access.h"
#include "neo4j_http_api_client.h"
#include "utilities/async_routine.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/maps_util.h"
#include "utilities/lists_vectors_util.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;
using QueryStatement = Neo4jHttpApiClient::QueryStatement;
using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

BoardsDataAccess::BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_)
        : AbstractBoardsDataAccess()
        , neo4jHttpApiClient(neo4jHttpApiClient_) {
}

void BoardsDataAccess::getWorkspaces(
        std::function<void (bool, const QHash<int, Workspace> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (w:Workspace)
                    OPTIONAL MATCH (w)-[:HAS]->(b:Board)
                    RETURN w, collect(b.id) AS boardIds
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto result = queryResponse.getResult().value();
                QHash<int, Workspace> idToWorkspace;
                for (int r = 0; r < result.rowCount(); ++r) {
                    const std::optional<QJsonObject> objectOpt = result.objectValueAt(r, "w");
                    const std::optional<QJsonArray> boardIdsOpt = result.arrayValueAt(r, "boardIds");
                    if (!objectOpt.has_value() || !boardIdsOpt.has_value())
                        continue;

                    const int id = objectOpt.value().value("id").toInt(-1);
                    if (id == -1)
                        continue;

                    Workspace workspace;
                    {
                        workspace.updateNodeProperties(objectOpt.value());

                        // populate `workspace.boardIds`
                        const QJsonArray boardIds = boardIdsOpt.value();
                        for (const QJsonValue &v: boardIds)
                            workspace.boardIds.insert(v.toInt(-1));
                    }
                    idToWorkspace.insert(id, workspace);
                }

                callback(true, idToWorkspace);
            },
            callbackContext
    );
}

void BoardsDataAccess::getWorkspacesListProperties(
        std::function<void (bool, WorkspacesListProperties)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (wl:WorkspacesList)
                    RETURN wl;
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.rowCount() == 0) {
                    // reply with default
                    callback(true, WorkspacesListProperties {});
                    return;
                }

                const auto objectOpt = result.objectValueAt(0, "wl");
                if (!objectOpt.has_value()) {
                    callback(false, {});
                    return;
                }

                WorkspacesListProperties properties;
                properties.update(objectOpt.value());
                callback(true, properties);
            },
            callbackContext
    );
}

void BoardsDataAccess::getBoardIdsAndNames(
        std::function<void (bool, const QHash<int, QString> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board)
                    RETURN b.id AS id, b.name AS name;
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                const auto result = queryResponse.getResult().value();
                QHash<int, QString> idToName;
                for (int r = 0; r < result.rowCount(); ++r) {
                    std::optional<int> id = result.intValueAt(r, "id");
                    std::optional<QString> name = result.stringValueAt(r, "name");
                    if (id.has_value() && name.has_value())
                        idToName.insert(id.value(), name.value());
                }

                callback(true, idToName);
            },
            callbackContext
    );
}

void BoardsDataAccess::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        bool hasError {false};
        std::optional<Board> boardData;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, boardId]() {
        // ==== 1st query ====
        neo4jHttpApiClient->queryDb(
                QueryStatement {
                    R"!(
                        MATCH (b:Board {id: $boardId})
                        RETURN b.id AS id, b AS data, 'board' AS what

                        UNION

                        MATCH (b:Board {id: $boardId})
                        MATCH (b)-[:HAS]->(n:NodeRect)-[:SHOWS]->(c:Card)
                        RETURN c.id AS id, n AS data, 'cardId-nodeRect' AS what

                        UNION

                        MATCH (b:Board {id: $boardId})
                        MATCH (b)-[:HAS]->(dv:DataViewBox)-[:SHOWS]->(q:CustomDataQuery)
                        RETURN q.id AS id, dv AS data, 'customDataQueryId-dataViewBox' AS what

                        UNION

                        MATCH (b:Board {id: $boardId})
                            (()-[:GROUP_ITEM]->(:GroupBox)) {1,}
                            (g:GroupBox)
                        RETURN g.id AS id, g AS data, 'groupBox' AS what
                    )!",
                    QJsonObject {{"boardId", boardId}}
                },
                // callback
                [routine](const QueryResponseSingleResult &queryResponse) {
                    if (!queryResponse.getResult().has_value()) {
                        routine->hasError = true;
                        routine->skipToFinalStep();
                        return;
                    }

                    const auto queryResult = queryResponse.getResult().value();
                    if (queryResult.isEmpty()) { // board not found (not an error)
                        routine->skipToFinalStep();
                    }

                    //
                    Board board;
                    bool gotBoardProperties = false;
                    bool hasError = false;

                    for (int r = 0; r < queryResult.rowCount(); ++r) {
                        const auto idOpt = queryResult.intValueAt(r, "id");
                        const auto dataOpt = queryResult.objectValueAt(r, "data");
                        const auto whatOpt = queryResult.stringValueAt(r, "what");

                        if (!idOpt.has_value() || !dataOpt.has_value() || !whatOpt.has_value()) {
                            hasError = true;
                            break;
                        }

                        if (whatOpt.value() == "board") {
                            gotBoardProperties = true;
                            board.updateNodeProperties(dataOpt.value());
                        }
                        else if (whatOpt.value() == "cardId-nodeRect") {
                            const int cardId = idOpt.value();
                            const std::optional<NodeRectData> nodeRectData
                                    = NodeRectData::fromJson(dataOpt.value());
                            if (!nodeRectData.has_value()) {
                                hasError = true;
                                break;
                            }
                            board.cardIdToNodeRectData.insert(cardId, nodeRectData.value());
                        }
                        else if (whatOpt.value() == "customDataQueryId-dataViewBox") {
                            const int customDataQueryId = idOpt.value();
                            const std::optional<DataViewBoxData> dataViewBoxData
                                    = DataViewBoxData::fromJson(dataOpt.value());
                            if (!dataViewBoxData.has_value()) {
                                hasError = true;
                                break;
                            }
                            board.customDataQueryIdToDataViewBoxData.insert(
                                    customDataQueryId, dataViewBoxData.value());
                        }
                        else if (whatOpt.value() == "groupBox") {
                            const int groupBoxId = idOpt.value();

                            GroupBoxData groupBoxData;
                            bool ok = groupBoxData.updateNodeProperties(dataOpt.value());
                            if (!ok) {
                                hasError = true;
                                break;
                            }

                            board.groupBoxIdToData.insert(groupBoxId, groupBoxData);
                        }
                        else {
                            Q_ASSERT(false); // case not implemented
                        }
                    }

                    if (!gotBoardProperties) {
                        qWarning().noquote() << "board properties not found";
                        hasError = true;
                    }

                    //
                    if (hasError) {
                        routine->hasError = true;
                        routine->skipToFinalStep();
                        return;
                    }

                    routine->boardData = board;
                    routine->nextStep();
                },
                routine
        );
    }, routine);

    routine->addStep([this, routine]() {
        // ==== 2nd query ====
        // For each group-box, get the group-boxes and cards (NodeRect's) contained in it.
        Q_ASSERT(routine->boardData.has_value());
        const QJsonArray groupBoxIds = toJsonArray(
                keySet(routine->boardData.value().groupBoxIdToData));

        neo4jHttpApiClient->queryDb(
                QueryStatement {
                    R"!(
                        MATCH (g:GroupBox)
                        WHERE g.id in $groupBoxIds
                        WITH g
                        OPTIONAL MATCH (g)-[:GROUP_ITEM]->(g1:GroupBox)
                        RETURN g.id AS groupBoxId, collect(g1.id) AS items, 'groupBoxes' AS itemType

                        UNION

                        MATCH (g:GroupBox)
                        WHERE g.id in $groupBoxIds
                        WITH g
                        OPTIONAL MATCH (g)-[:GROUP_ITEM]->(:NodeRect)-[:SHOWS]->(c:Card)
                        RETURN g.id AS groupBoxId, collect(c.id) AS items, 'cards' AS itemType
                    )!",
                    QJsonObject {{"groupBoxIds", groupBoxIds}}
                },
                // callback
                [routine](const QueryResponseSingleResult &queryResponse) {
                    if (!queryResponse.getResult().has_value()) {
                        routine->hasError = true;
                        routine->skipToFinalStep();
                        return;
                    }

                    Board &boardData = routine->boardData.value();

                    const auto result = queryResponse.getResult().value();
                    bool hasError = false;
                    for (int r = 0; r < result.rowCount(); ++r) {
                        const auto groupBoxIdOpt = result.intValueAt(r, "groupBoxId");
                        const auto itemsOpt = result.arrayValueAt(r, "items");
                        const auto itemTypeOpt = result.stringValueAt(r, "itemType");

                        if (!groupBoxIdOpt.has_value()
                                || !itemsOpt.has_value() || !itemTypeOpt.has_value()) {
                            hasError = true;
                            break;
                        }

                        const int groupBoxId = groupBoxIdOpt.value();
                        if (itemTypeOpt.value() == "groupBoxes") {
                            boardData.groupBoxIdToData[groupBoxId].childGroupBoxes
                                    = toIntSet(itemsOpt.value());
                        }
                        else if (itemTypeOpt.value() == "cards") {
                            boardData.groupBoxIdToData[groupBoxId].childCards
                                    = toIntSet(itemsOpt.value());
                        }
                        else {
                            Q_ASSERT(false); // case not implemented
                        }
                    }

                    //
                    if (hasError) {
                        routine->hasError = true;
                        routine->skipToFinalStep();
                        return;
                    }
                    routine->nextStep();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // ==== final step ====
        if (routine->hasError)
            callback(false, std::nullopt);
        else
            callback(true, routine->boardData);

        routine->nextStep();
    }, callbackContext);

    routine->start();
}

void BoardsDataAccess::createNewWorkspaceWithId(
        const int workspaceId, const Workspace &workspace,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    Q_ASSERT(workspace.boardIds.isEmpty()); // new workspace should have no board

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    CREATE (ws:Workspace {id: $workspaceId})
                    SET ws += $propertiesMap
                    RETURN ws.id AS id
                )!",
                QJsonObject {
                    {"workspaceId", workspaceId},
                    {"propertiesMap", workspace.getNodePropertiesJson()}
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    callback(false);
                    return;
                }

                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::updateWorkspaceNodeProperties(
        const int workspaceId, const WorkspaceNodePropertiesUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (ws:Workspace {id: $workspaceId})
                    SET ws += $propertiesMap
                    RETURN ws.id
                )!",
                QJsonObject {
                    {"workspaceId", workspaceId},
                    {"propertiesMap", update.toJson()}
                }
            },
            // callback
            [callback, workspaceId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    qWarning().noquote()
                            << QString("workspace %1 not found or properties could not be written")
                               .arg(workspaceId);
                    callback(false);
                    return;
                }

                qInfo().noquote() << QString("updated properties of workspace %1").arg(workspaceId);
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::removeWorkspace(
        const int workspaceId, std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, workspaceId]() {
        // 2. remove NodeRect & DataViewBox
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:Workspace {id: $workspaceId})-[:HAS]->(:Board)
                            -[:HAS]->(n:NodeRect|DataViewBox)
                        DETACH DELETE n
                    )!",
                    QJsonObject {{"workspaceId", workspaceId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, workspaceId]() {
        // 3. remove Board
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:Workspace {id: $workspaceId})-[:HAS]->(b:Board)
                        DETACH DELETE b
                    )!",
                    QJsonObject {{"workspaceId", workspaceId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, workspaceId]() {
        // 4. remove workspace
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (ws:Workspace {id: $workspaceId})
                        DETACH DELETE ws
                    )!",
                    QJsonObject {{"workspaceId", workspaceId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine]() {
        // 5. commit transaction
        routine->transaction->commit(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // 6. final step
        ContinuationContext context(routine);
        routine->transaction->deleteLater();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void BoardsDataAccess::updateWorkspacesListProperties(
        const WorkspacesListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MERGE (wl:WorkspacesList)
                    SET wl += $propertiesMap
                    RETURN wl
                )!",
                QJsonObject {{"propertiesMap", propertiesUpdate.toJson()}}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                const bool ok = !result.isEmpty();
                callback(ok);
            },
            callbackContext
    );
}

void BoardsDataAccess::requestNewBoardId(
        std::function<void (bool, int)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:LastUsedId {itemType: 'Board'})
                    SET n.value = n.value + 1
                    RETURN n.value AS boardId
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value())
                {
                    callback(false, -1);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                const std::optional<int> boardId = result.intValueAt(0, "boardId");
                if (!boardId.has_value()) {
                    callback(false, -1);
                    return;
                }

                callback(true, boardId.value());
            },
            callbackContext
    );
}

void BoardsDataAccess::createNewBoardWithId(
        const int boardId, const Board &board, const int workspaceId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    Q_ASSERT(board.cardIdToNodeRectData.isEmpty()); // new board should have no NodeRect

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    CREATE (b:Board {id: $boardId})
                    SET b += $propertiesMap
                    WITH b
                    OPTIONAL MATCH (ws:Workspace {id: $workspaceId})
                    CALL apoc.do.when(
                        ws IS NOT NULL,
                        'CREATE (ws)-[:HAS]->(b) RETURN true AS relCreated',
                        'RETURN false AS relCreated',
                        {b: b, ws: ws}
                    ) YIELD value
                    RETURN b.id AS id, value.relCreated AS relCreated
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"workspaceId", workspaceId},
                    {"propertiesMap", board.getNodePropertiesJson()}
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    callback(false);
                    return;
                }

                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                    SET b += $propertiesMap
                    RETURN b.id
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"propertiesMap", propertiesUpdate.toJson()}
                }
            },
            // callback
            [callback, boardId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                bool hasError = false;

                if (queryResponse.hasNetworkOrDbError())
                    hasError = true;

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    qWarning().noquote() << QString("board %1 not found").arg(boardId);
                    hasError = true;
                }

                qInfo().noquote() << QString("updated properties of board %1").arg(boardId);
                callback(!hasError);
            },
            callbackContext
    );
}

void BoardsDataAccess::removeBoard(
        const int boardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, boardId]() {
        // 2. remove NodeRect & DataViewBox nodes
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:Board {id: $boardId})-[:HAS]->(n:NodeRect|DataViewBox)
                        DETACH DELETE n
                    )!",
                    QJsonObject {{"boardId", boardId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, boardId]() {
        // 3. remove board node
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (b:Board {id: $boardId})
                        DETACH DELETE b
                    )!",
                    QJsonObject {{"boardId", boardId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine]() {
        // 4. commit transaction
        routine->transaction->commit(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // 5. final step
        ContinuationContext context(routine);
        routine->transaction->deleteLater();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void BoardsDataAccess::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                            -[:HAS]->(n:NodeRect)
                            -[:SHOWS]->(c:Card {id: $cardId})
                    SET n += $propertiesMap
                    RETURN n
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"cardId", cardId},
                    {"propertiesMap", update.toJson()}
                }
            },
            // callback
            [callback, boardId, cardId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                bool hasError = false;
                if (result.isEmpty()) {
                    hasError = true;
                    qWarning().noquote()
                            << QString("NodeRect for board %1 & card %2 is not found")
                               .arg(boardId).arg(cardId);

                }

                callback(!hasError);
            },
            callbackContext
    );
}

void BoardsDataAccess::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                    MATCH (c:Card {id: $cardId})
                    MERGE (b)-[:HAS]->(n:NodeRect)-[:SHOWS]->(c)
                    ON CREATE
                        SET n += $propertiesMap, n._is_created_ = true
                    ON MATCH
                        SET n._is_created_ = false
                    WITH n, n._is_created_ AS isCreated
                    REMOVE n._is_created_
                    RETURN isCreated
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"cardId", cardId},
                    {"propertiesMap", nodeRectData.toJson()}
                }
            },
            // callback
            [callback, boardId, cardId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote()
                            << QString("Board %1 or card %2 does not exist")
                               .arg(boardId).arg(cardId);
                    callback(false);
                    return;
                }

                std::optional<bool> isCreated = result.boolValueAt(0, "isCreated");
                if (!isCreated.has_value()) {
                    callback(false);
                    return;
                }

                if (!isCreated.value()) {
                    qWarning().noquote()
                            << QString("NodeRect for board %1 & card %2 already exists")
                               .arg(boardId).arg(cardId);
                    callback(false);
                    return;
                }

                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::removeNodeRect(
        const int boardId, const int cardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (:Board {id: $boardId})
                          -[:HAS]->(n:NodeRect)
                          -[:SHOWS]->(:Card {id: $cardId})
                    DETACH DELETE n
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"cardId", cardId}
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::createDataViewBox(
        const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                    MATCH (q:CustomDataQuery {id: $customDataQueryId})
                    MERGE (b)-[:HAS]->(box:DataViewBox)-[:SHOWS]->(q)
                    ON CREATE
                        SET box += $propertiesMap, box._is_created_ = true
                    ON MATCH
                        SET box._is_created_ = false
                    WITH box, box._is_created_ AS isCreated
                    REMOVE box._is_created_
                    RETURN isCreated
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"customDataQueryId", customDataQueryId},
                    {"propertiesMap", dataViewBoxData.toJson()}
                }
            },
            // callback
            [callback, boardId, customDataQueryId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote()
                            << QString("Board %1 or custom-data-query %2 does not exist")
                               .arg(boardId).arg(customDataQueryId);
                    callback(false);
                    return;
                }

                std::optional<bool> isCreated = result.boolValueAt(0, "isCreated");
                if (!isCreated.has_value()) {
                    callback(false);
                    return;
                }

                if (!isCreated.value()) {
                    qWarning().noquote()
                            << QString("DataViewBox for board %1 & custom-data-query %2 "
                                       "already exists")
                               .arg(boardId).arg(customDataQueryId);
                    callback(false);
                    return;
                }

                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::updateDataViewBoxProperties(
        const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                            -[:HAS]->(box:DataViewBox)
                            -[:SHOWS]->(q:CustomDataQuery {id: $customDataQueryId})
                    SET box += $propertiesMap
                    RETURN box
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"customDataQueryId", customDataQueryId},
                    {"propertiesMap", update.toJson()}
                }
            },
            // callback
            [callback, boardId, customDataQueryId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote()
                            << QString("DataViewBox for board %1 & custo-data-query %2 "
                                       "is not found")
                               .arg(boardId).arg(customDataQueryId);
                    callback(false);
                    return;
                }

                qInfo().noquote() << "updated DataViewBox properties";
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::removeDataViewBox(
        const int boardId, const int customDataQueryId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (:Board {id: $boardId})
                          -[:HAS]->(box:DataViewBox)
                          -[:SHOWS]->(:CustomDataQuery {id: $customDataQueryId})
                    DETACH DELETE box
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"customDataQueryId", customDataQueryId}
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::createTopLevelGroupBoxWithId(
        const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (b:Board {id: $boardId})
                    MERGE (b)-[:GROUP_ITEM]->(g:GroupBox {id: $groupBoxId})
                    ON CREATE
                        SET g += $propertiesMap, g._is_created_ = true
                    ON MATCH
                        SET g._is_created_ = false
                    WITH g, g._is_created_ AS isCreated
                    REMOVE g._is_created_
                    RETURN isCreated
                )!",
                QJsonObject {
                    {"boardId", boardId},
                    {"groupBoxId", groupBoxId},
                    {"propertiesMap", groupBoxData.getNodePropertiesJson()}
                }
            },
            // callback
            [callback, boardId, groupBoxId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote() << QString("Board %1 does not exist").arg(boardId);
                    callback(false);
                    return;
                }

                std::optional<bool> isCreated = result.boolValueAt(0, "isCreated");
                if (!isCreated.has_value()) {
                    callback(false);
                    return;
                }

                if (!isCreated.value()) {
                    qWarning().noquote() << QString("GroupBox %1 already exists").arg(groupBoxId);
                    callback(false);
                    return;
                }

                qInfo().noquote() << QString("top-level GroupBox %1 created").arg(groupBoxId);
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::updateGroupBoxProperties(
        const int groupBoxId, const GroupBoxNodePropertiesUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(MATCH (g:GroupBox {id: $id})
                    SET g += $propertiesMap
                    RETURN g.id
                )!",
                QJsonObject {
                    {"id", groupBoxId},
                    {"propertiesMap", update.toJson()}
                }
            },
            // callback:
            [callback, groupBoxId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                if (queryResponse.hasNetworkOrDbError()) {
                    callback(false);
                    return;
                }

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    qWarning().noquote()
                            << QString("group-box %1 not found or properties could not be set")
                               .arg(groupBoxId);
                    callback(false);
                    return;
                }

                qInfo().noquote()
                        << QString("updated properties of group-box %1").arg(groupBoxId);
                callback(true);
            },
            callbackContext
    );
}

void BoardsDataAccess::removeGroupBoxAndReparentChildItems(
        const int groupBoxId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId]() {
        // create relationships
        //     (parent of `groupBoxId`) -[:GROUP_ITEM]-> (child group-boxes of `groupBoxId`)
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (parent:GroupBox|Board)
                                -[:GROUP_ITEM]->(:GroupBox {id: $groupBoxId})
                                -[:GROUP_ITEM]->(childGroupBox:GroupBox)
                        MERGE (parent)-[:GROUP_ITEM]->(childGroupBox)
                    )!",
                    QJsonObject {{"groupBoxId", groupBoxId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId]() {
        // create relationships
        //     (parent group-box of `groupBoxId`) -[:GROUP_ITEM]-> (child NodeRect's of `groupBoxId`),
        // if the parent of `groupBoxId` is a group-box
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (parent:GroupBox)
                                -[:GROUP_ITEM]->(:GroupBox {id: $groupBoxId})
                                -[:GROUP_ITEM]->(childNodeRect:NodeRect)
                        MERGE (parent)-[:GROUP_ITEM]->(childNodeRect)
                    )!",
                    QJsonObject {{"groupBoxId", groupBoxId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId]() {
        // delete `groupBoxId`
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (g:GroupBox {id: $groupBoxId})
                        DETACH DELETE g
                    )!",
                    QJsonObject {{"groupBoxId", groupBoxId}}
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId]() {
        // commit transaction
        routine->transaction->commit(
                // callback
                [routine, groupBoxId](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                    else
                        qInfo().noquote() << QString("GroupBox %1 removed").arg(groupBoxId);
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // final step
        ContinuationContext context(routine);
        routine->transaction->deleteLater();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void BoardsDataAccess::addOrReparentNodeRectToGroupBox(
        const int cardId, const int newGroupBoxId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
        int boardId {-1};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, newGroupBoxId]() {
        // find the board where `newGroupBoxId` is in
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (b:Board)
                            (()-[:GROUP_ITEM]->(:GroupBox)) {1,}
                            (:GroupBox {id: $newGroupBoxId})
                        RETURN b.id AS boardId
                    )!",
                    QJsonObject {{"newGroupBoxId", newGroupBoxId}}
                },
                // callback
                [routine, newGroupBoxId](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);

                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    const auto result = queryResponse.getResult().value();
                    if (result.isEmpty()) { // `newGroupBoxId` not found
                        qWarning().noquote() << QString("group-box %1 not found").arg(newGroupBoxId);
                        context.setErrorFlag();
                        return;
                    }

                    const auto boardIdOpt = result.intValueAt(0, "boardId");
                    if (!boardIdOpt.has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    routine->boardId = boardIdOpt.value();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, cardId]() {
        // remove NodeRect from its original parent, if found
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:Board {id: $boardId})
                                -[:HAS]->(n:NodeRect)
                                -[:SHOWS]->(:Card {id: $cardId})
                        MATCH (:GroupBox)-[r:GROUP_ITEM]->(n)
                        DELETE r
                    )!",
                    QJsonObject {
                        {"boardId", routine->boardId},
                        {"cardId", cardId}
                    }
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &/*queryResponse*/) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, cardId, newGroupBoxId]() {
        // add NodeRect to `newGroupBoxId`
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:Board {id: $boardId})
                                -[:HAS]->(n:NodeRect)
                                -[:SHOWS]->(:Card {id: $cardId})
                        MATCH (gNew:GroupBox {id: $newGroupBoxId})
                        MERGE (gNew)-[:GROUP_ITEM]->(n)
                        RETURN gNew.id
                    )!",
                    QJsonObject {
                        {"boardId", routine->boardId},
                        {"cardId", cardId},
                        {"newGroupBoxId", newGroupBoxId}
                    }
                },
                // callback
                [routine, cardId](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);

                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    const auto result = queryResponse.getResult().value();
                    if (result.isEmpty()) { // NodeRect for (routine->boardId, cardId) not found
                        qWarning().noquote()
                                << QString("NodeRect for board %1 and card %2 is not found")
                                   .arg(routine->boardId).arg(cardId);
                        context.setErrorFlag();
                        return;
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, newGroupBoxId]() {
        // commit transaction
        routine->transaction->commit(
                // callback
                [routine, newGroupBoxId](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                    }
                    else {
                        qInfo().noquote()
                                << QString("NodeRect added or reparented to GroupBox %1")
                                   .arg(newGroupBoxId);
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // final step
        ContinuationContext context(routine);
        routine->transaction->deleteLater();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void BoardsDataAccess::reparentGroupBox(
        const int groupBoxId, const int newParentGroupBox,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    // \param groupBoxId: must exist
    // \param newParentGroupBox
    //           + if = -1: `groupBoxId` will be reparented to the board
    //           + if != -1: must be on the same board as `groupBoxId`, and must not be `groupBoxId`
    //                       or its descendant

    Q_ASSERT(callback);

    // ==== case: `newParentGroupBox` = -1 ====
    if (newParentGroupBox == -1) {
        // reparent `groupBoxId` to the board if its parent is a group-box
        neo4jHttpApiClient->queryDb(
                QueryStatement {
                    R"!(
                        MATCH (g:GroupBox {id: $groupBoxId})
                        RETURN 1 AS x

                        UNION

                        MATCH (:GroupBox)-[r:GROUP_ITEM]->(g:GroupBox {id: $groupBoxId})
                        MATCH (b:Board) (()-[:GROUP_ITEM]->(:GroupBox)) {1,} (g)
                        MERGE (b)-[:GROUP_ITEM]->(g)
                        DELETE r
                        RETURN 2 AS x
                    )!",
                    QJsonObject {
                        {"groupBoxId", groupBoxId},
                    }
                },
                // callback:
                [callback, groupBoxId](const QueryResponseSingleResult &queryResponse) {
                    if (!queryResponse.getResult().has_value()) {
                        callback(false);
                        return;
                    }

                    if (queryResponse.hasNetworkOrDbError()) {
                        callback(false);
                        return;
                    }

                    const auto queryResult = queryResponse.getResult().value();
                    if (queryResult.isEmpty()) {
                        qWarning().noquote() << QString("group-box %1 not found").arg(groupBoxId);
                        callback(false);
                        return;
                    }

                    qInfo().noquote() << QString("reparented GroupBox %1").arg(groupBoxId);
                    callback(true);
                },
                callbackContext
        );

        return;
    }

    // ==== case: `newParentGroupBox` != -1 ====
    Q_ASSERT(newParentGroupBox != -1);

    if (newParentGroupBox == groupBoxId) {
        qWarning().noquote() << QString("cannot reparent group-box %1 to itself").arg(groupBoxId);
        invokeAction(callbackContext, [callback]() {
            callback(false);
        });
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
        int boardId {-1};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId, newParentGroupBox]() {
        // check `groupBoxId` & `newParentGroupBox` belong to the same board
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (b:Board)
                                (()-[:GROUP_ITEM]->(:GroupBox)) {1,}
                                (:GroupBox {id: $groupBoxId})
                        MATCH (b)
                                (()-[:GROUP_ITEM]->(:GroupBox)) {1,}
                                (:GroupBox {id: $newParentGroupBox})
                        RETURN b.id
                    )!",
                    QJsonObject {
                        {"groupBoxId", groupBoxId},
                        {"newParentGroupBox", newParentGroupBox}
                    }
                },
                // callback
                [=](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);

                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    const auto result = queryResponse.getResult().value();
                    if (result.isEmpty()) {
                        qWarning().noquote()
                                << QString("group-boxes %1 & %2 do not belong to the same board")
                                   .arg(groupBoxId, newParentGroupBox);
                        context.setErrorFlag();
                        return;
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId, newParentGroupBox]() {
        // check that `newParentGroupBox` is not a descendant of `groupBoxId`
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:GroupBox {id: $groupBoxId})
                                (()-[:GROUP_ITEM]->(:GroupBox)) {1,}
                                (:GroupBox {id: $newParentGroupBox})
                        RETURN 1
                    )!",
                    QJsonObject {
                        {"groupBoxId", groupBoxId},
                        {"newParentGroupBox", newParentGroupBox}
                    }
                },
                // callback
                [=](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);

                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    const auto result = queryResponse.getResult().value();
                    if (!result.isEmpty()) {
                        qWarning().noquote()
                                << QString("group-boxes %1 is a descendant of group-box %2")
                                   .arg(newParentGroupBox, groupBoxId);
                        context.setErrorFlag();
                        return;
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId, newParentGroupBox]() {
        // reparent `groupBoxId`
        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (:GroupBox|Board)
                                -[r:GROUP_ITEM]->(:GroupBox {id: $groupBoxId})
                        DELETE r
                        RETURN 1 AS x

                        UNION

                        MATCH (g:GroupBox {id: $groupBoxId})
                        MATCH (gNew:GroupBox {id: $newParentGroupBox})
                        MERGE (gNew)-[:GROUP_ITEM]->(g)
                        RETURN 2 AS x
                    )!",
                    QJsonObject {
                        {"groupBoxId", groupBoxId},
                        {"newParentGroupBox", newParentGroupBox},
                    }
                },
                // callback
                [routine](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);
                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, groupBoxId]() {
        // commit transaction
        routine->transaction->commit(
                // callback
                [routine, groupBoxId](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                    else
                        qInfo().noquote() << QString("reparented GroupBox %1").arg(groupBoxId);
                },
                routine
        );
    }, routine);

    routine->addStep([routine, callback]() {
        // final step
        ContinuationContext context(routine);
        routine->transaction->deleteLater();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void BoardsDataAccess::removeNodeRectFromGroupBox(
        const int cardId, std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (:GroupBox)
                            -[r:GROUP_ITEM]->(:NodeRect)
                            -[:SHOWS]->(:Card {id: $cardId})
                    DELETE r
                )!",
                QJsonObject {
                    {"cardId", cardId}
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }
                qInfo().noquote() << QString("removed NodeRect from GroupBox");
                callback(true);
            },
            callbackContext
    );
}
