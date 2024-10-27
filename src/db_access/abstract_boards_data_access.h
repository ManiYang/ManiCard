#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

#include <functional>
#include <optional>
#include <QPointer>
#include "models/board.h"
#include "models/boards_list_properties.h"
#include "models/node_rect_data.h"
#include "models/data_view_box_data.h"

class AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccessReadOnly();

    virtual void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
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

    // ==== boards list ====

    //!
    //! This operation is atomic.
    //!
    virtual void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== board ====

    virtual void requestNewBoardId(
            std::function<void (bool ok, int boardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    //!
    //! Board with ID \e boardId must not already exist. This operation is atomic.
    //! \param boardId:
    //! \param board: should have no NodeRect
    //! \param callback
    //! \param callbackContext
    //!
    virtual void createNewBoardWithId(
            const int boardId, const Board &board,
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
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
