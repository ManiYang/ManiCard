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
                    callback(false, {});
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

void BoardsDataAccess::getBoardsListProperties(
        std::function<void (bool ok, BoardsListProperties properties)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:BoardsList)
                    RETURN n;
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, BoardsListProperties {});
                    return;
                }

                const auto result = queryResponse.getResult().value();
                std::optional<QJsonObject> propertiesObj = result.objectValueAt(0, "n");
                if (!propertiesObj.has_value()) {
                    // node not found, reply with default
                    callback(true, BoardsListProperties {});
                }
                else {
                    BoardsListProperties properties;
                    properties.update(propertiesObj.value());
                    callback(true, properties);
                }
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
                    RETURN b AS board,
                           null AS nodeRect, null AS cardId,
                           null AS dataViewBox, null AS customDataQueryId

                    UNION

                    MATCH (b:Board {id: $boardId})
                    MATCH (b)-[:HAS]->(n:NodeRect)-[:SHOWS]->(c:Card)
                    RETURN null AS board,
                           n AS nodeRect, c.id AS cardId,
                           null AS dataViewBox, null AS customDataQueryId

                    UNION

                    MATCH (b:Board {id: $boardId})
                    MATCH (b)-[:HAS]->(box:DataViewBox)-[:SHOWS]->(q:CustomDataQuery)
                    RETURN null AS board,
                           null AS nodeRect, null AS cardId,
                           box AS dataViewBox, q.id AS customDataQueryId
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
                    const auto boardValue = queryResult.valueAt(r, "board");
                    const auto nodeRectValue = queryResult.valueAt(r, "nodeRect");
                    const auto cardIdValue = queryResult.valueAt(r, "cardId");
                    const auto dataViewBoxValue = queryResult.valueAt(r, "dataViewBox");
                    const auto customDataQueryIdValue = queryResult.valueAt(r, "customDataQueryId");

                    if (!boardValue.isNull()) {
                        gotBoardProperties = true;

                        if (!boardValue.isObject()) {
                            hasError = true;
                            break;
                        }

                        board.updateNodeProperties(boardValue.toObject());
                    }
                    else if (!nodeRectValue.isNull()) {
                        if (!nodeRectValue.isObject() || !cardIdValue.isDouble()) {
                            hasError = true;
                            break;
                        }

                        const std::optional<NodeRectData> nodeRectData
                                = NodeRectData::fromJson(nodeRectValue.toObject());
                        if (!nodeRectData.has_value()) {
                            hasError = true;
                            break;
                        }

                        board.cardIdToNodeRectData.insert(cardIdValue.toInt(), nodeRectData.value());
                    }
                    else if (!dataViewBoxValue.isNull()) {
                        if (!dataViewBoxValue.isObject() || !customDataQueryIdValue.isDouble()) {
                            hasError = true;
                            break;
                        }

                        const std::optional<DataViewBoxData> dataViewBoxData
                                = DataViewBoxData::fromJson(dataViewBoxValue.toObject());
                        if (!dataViewBoxData.has_value()) {
                            hasError = true;
                            break;
                        }

                        board.customDataQueryIdToDataViewBoxData.insert(
                                customDataQueryIdValue.toInt(), dataViewBoxData.value());
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

void BoardsDataAccess::updateBoardsListProperties(
        const BoardsListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:BoardsList)
                    SET n += $propertiesMap
                    RETURN n
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
