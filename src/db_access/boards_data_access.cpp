#include "neo4j_http_api_client.h"
#include "boards_data_access.h"

using QueryStatement = Neo4jHttpApiClient::QueryStatement;
using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

BoardsDataAccess::BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_)
        : AbstractBoardsDataAccess()
        , neo4jHttpApiClient(neo4jHttpApiClient_) {
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
                    board.setNodeProperties(boardPropertiesObj.value());

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
