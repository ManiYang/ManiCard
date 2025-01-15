#ifndef BOARDSDATAACCESS_H
#define BOARDSDATAACCESS_H

#include "abstract_boards_data_access.h"

class Neo4jHttpApiClient;

class BoardsDataAccess : public AbstractBoardsDataAccess
{
public:
    explicit BoardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient_);

    // ==== read operations ====

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

    // ==== write operations ====

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
            const int groupBoxId, const GroupBoxNodePropertiesUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeGroupBoxAndReparentChildItems(
            const int groupBoxId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void addOrReparentNodeRectToGroupBox(
            const int cardId, const int newGroupBoxId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void reparentGroupBox(
            const int groupBoxId, const int newParentGroupBox,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeNodeRectFromGroupBox(
            const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // BOARDSDATAACCESS_H
