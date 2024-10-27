#ifndef BOARDSDATAACCESS_H
#define BOARDSDATAACCESS_H

#include "abstract_boards_data_access.h"

class Neo4jHttpApiClient;

class BoardsDataAccess : public AbstractBoardsDataAccess
{
public:
    explicit BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_);

    // ==== read operations ====

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) override;

    // ==== write operations ====

    void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void requestNewBoardId(
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext) override;

    void createNewBoardWithId(
            const int boardId, const Board &board,
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

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // BOARDSDATAACCESS_H
