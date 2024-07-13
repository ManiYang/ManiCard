#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

#include <functional>
#include <optional>
#include <QPointer>
#include "models/board.h"

class AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccessReadOnly();

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
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
