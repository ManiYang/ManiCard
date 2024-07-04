#include "abstract_boards_data_access.h"

AbstractBoardsDataAccessReadOnly::AbstractBoardsDataAccessReadOnly(QObject *parent)
    : QObject(parent)
{}

AbstractBoardsDataAccess::AbstractBoardsDataAccess(QObject *parent)
    : AbstractBoardsDataAccessReadOnly(parent)
{}
