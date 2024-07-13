#ifndef BOARDSDATAACCESS_H
#define BOARDSDATAACCESS_H

#include "abstract_boards_data_access.h"

class Neo4jHttpApiClient;

class BoardsDataAccess : public AbstractBoardsDataAccess
{
public:
    explicit BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_);

    void getBoardData(
                const int boardId,
                std::function<void (bool ok, std::optional<Board> board)> callback,
                QPointer<QObject> callbackContext) override;

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // BOARDSDATAACCESS_H
