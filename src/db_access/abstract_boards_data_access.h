#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

class AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccessReadOnly();
};

class AbstractBoardsDataAccess : public AbstractBoardsDataAccessReadOnly
{
public:
    explicit AbstractBoardsDataAccess();
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
