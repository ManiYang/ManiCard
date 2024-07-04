#ifndef ABSTRACT_BOARDS_DATA_ACCESS_H
#define ABSTRACT_BOARDS_DATA_ACCESS_H

#include <QObject>

class AbstractBoardsDataAccessReadOnly : public QObject
{
    Q_OBJECT
public:
    explicit AbstractBoardsDataAccessReadOnly(QObject *parent = nullptr);
};

class AbstractBoardsDataAccess : public AbstractBoardsDataAccessReadOnly
{
    Q_OBJECT
public:
    explicit AbstractBoardsDataAccess(QObject *parent = nullptr);
};

#endif // ABSTRACT_BOARDS_DATA_ACCESS_H
