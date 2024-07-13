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

    void getBoardsOrdering(
            std::function<void (bool ok, const QVector<int> &boardsOrdering)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) override;

    // ==== read operations ====

    void updateBoardsOrdering(
            const QVector<int> boardsOrdering,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // BOARDSDATAACCESS_H
