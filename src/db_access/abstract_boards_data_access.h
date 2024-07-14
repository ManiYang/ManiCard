#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

#include <functional>
#include <optional>
#include <QPointer>
#include "models/board.h"
#include "models/boards_list_properties.h"

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

    //!
    //! This operation is atomic.
    //!
    virtual void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The board must exist. This operation is atomic.
    //!
    virtual void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
