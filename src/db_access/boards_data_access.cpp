#include "boards_data_access.h"
#include "neo4j_http_api_client.h"
#include "utilities/json_util.h"

using QueryStatement = Neo4jHttpApiClient::QueryStatement;
using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

BoardsDataAccess::BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_)
        : AbstractBoardsDataAccess()
        , neo4jHttpApiClient(neo4jHttpApiClient_) {
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

void BoardsDataAccess::getBoardsOrdering(
        std::function<void (bool, const QVector<int> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:BoardsList)
                    RETURN n.boardsOrdering AS ordering;
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
                std::optional<QJsonArray> array = result.arrayValueAt(0, "ordering");
                if (!array.has_value()) {
                    callback(false, {});
                    return;
                }

                callback(true, toIntVector(array.value(), -1));
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
                    RETURN b AS board, null AS nodeRect, null AS cardId
                    UNION
                    MATCH (b:Board {id: $boardId})
                    MATCH (b)-[:HAS]->(n:NodeRect)-[:SHOWS]->(c:Card)
                    RETURN null AS board, n AS nodeRect, c.id AS cardId
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

                Board board;
                bool hasError = false;

                // row 0
                std::optional<QJsonObject> boardPropertiesObj
                        = queryResult.objectValueAt(0, "board");
                if (!boardPropertiesObj.has_value())
                    hasError = true;
                else
                    board.updateNodeProperties(boardPropertiesObj.value());

                // row >= 1
                for (int r = 1; r < queryResult.rowCount(); ++r) {
                    std::optional<QJsonObject>nodeRectObj
                            = queryResult.objectValueAt(r, "nodeRect");
                    std::optional<int> cardId = queryResult.intValueAt(r, "cardId");

                    if (!nodeRectObj.has_value() || !cardId.has_value()) {
                        hasError = true;
                        continue;
                    }

                    //
                    const std::optional<NodeRectData> nodeRectData
                            = NodeRectData::fromJson(nodeRectObj.value());

                    if (!nodeRectData.has_value()) {
                        hasError = true;
                        continue;
                    }

                    //
                    board.cardIdToNodeRectData.insert(cardId.value(), nodeRectData.value());
                }

                //
                callback(!hasError, board);
            },
            callbackContext
    );
}

void BoardsDataAccess::updateBoardsOrdering(
        const QVector<int> boardsOrdering, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:BoardsList)
                    SET n.boardsOrdering = $orderingArray
                    RETURN n
                )!",
                QJsonObject {{"orderingArray", toJsonArray(boardsOrdering)}}
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

                callback(!hasError);
            },
            callbackContext
    );
}
