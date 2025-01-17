#include "boards_data_access.h"
#include "neo4j_http_api_client.h"
#include "utilities/async_routine.h"
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
                    MATCH (b)-[:HAS]->(g:GroupBox)
                    RETURN g.id AS id, g AS data, 'groupBox' AS what
                )!",
                QJsonObject {{"boardId", boardId}}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, std::nullopt);
                    return;
                }

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    callback(true, std::nullopt); // board not found
                    return;
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
                        const std::optional<GroupBoxData> groupBoxData
                                = GroupBoxData::fromJson(dataOpt.value());
                        if (!groupBoxData.has_value()) {
                            hasError = true;
                            break;
                        }
                        board.groupBoxIdToData.insert(groupBoxId, groupBoxData.value());
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
                if (!hasError)
                    callback(true, board);
                else
                    callback(false, std::nullopt);
            },
            callbackContext
    );
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
