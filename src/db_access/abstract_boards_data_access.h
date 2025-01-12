#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

#include <functional>
#include <optional>
#include <QPointer>
#include <QVector>
#include "models/board.h"
#include "models/data_view_box_data.h"
#include "models/node_rect_data.h"
#include "models/workspace.h"
#include "models/workspaces_list_properties.h"

class AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccessReadOnly();

    virtual void getWorkspaces(
            std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) = 0;

    //!
    //! \param boardId
    //! \param callback: argument \e board will be \e nullopt if the board ID is not found
    //! \param callbackContext
    //!
    virtual void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) = 0;
};

class AbstractBoardsDataAccess : public AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccess();

    // ==== workspace ====

    //!
    //! Workspace with ID \e workspaceId must not already exist. This operation is atomic.
    //! \param workspaceId
    //! \param workspace: must have no boards
    //! \param callback
    //! \param callbackContext
    //!
    virtual void createNewWorkspaceWithId(
            const int workspaceId, const Workspace &workspace,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The workspace must exist. This operation is atomic.
    //!
    virtual void updateWorkspaceNodeProperties(
            const int workspaceId, const WorkspaceNodePropertiesUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! Also removes all boards that the workspace has.
    //! This operation is atomic and idempotent.
    //!
    virtual void removeWorkspace(
            const int workspaceId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! This operation is atomic.
    //!
    virtual void updateWorkspacesListProperties(
            const WorkspacesListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== board ====

    virtual void requestNewBoardId(
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    //!
    //! Board with ID \e boardId must not already exist. This operation is atomic.
    //! \param boardId:
    //! \param board: must have no NodeRect
    //! \param workspaceId: If it exists, the relationship (:Workspace)-[:HAS]->(:Board) will be
    //!                     created. It is not an error if \e workspaceId does not exist.
    //! \param callback
    //! \param callbackContext
    //!
    virtual void createNewBoardWithId(
            const int boardId, const Board &board, const int workspaceId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The board must exist. This operation is atomic.
    //!
    virtual void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! Also removes all NodeRects that the board has.
    //! This operation is atomic and idempotent.
    //!
    virtual void removeBoard(
            const int boardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== NodeRect ====

    //!
    //! The board and card must already exist, and the NodeRect must not already exist.
    //! The operation is atomic.
    //!
    virtual void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The NodeRect must exist. This operation is atomic.
    //!
    virtual void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! This operation is atomic and idempotent.
    //!
    virtual void removeNodeRect(
            const int boardId, const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== DataViewBox ====

    //!
    //! The board and custum data query must already exist, and the DataViewBox must not already
    //! exist.
    //! The operation is atomic.
    //!
    virtual void createDataViewBox(
            const int boardId, const int customDataQueryId,
            const DataViewBoxData &dataViewBoxData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The DataViewBox must exist. This operation is atomic.
    //!
    virtual void updateDataViewBoxProperties(
            const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! This operation is atomic and idempotent.
    //!
    virtual void removeDataViewBox(
            const int boardId, const int customDataQueryId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== GroupBox ====

    //!
    //! The GroupBox must exist. This operation is atomic.
    //!
    virtual void updateGroupBoxProperties(
            const int groupBoxId, const GroupBoxDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
