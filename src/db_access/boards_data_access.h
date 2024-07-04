#ifndef BOARDSDATAACCESS_H
#define BOARDSDATAACCESS_H

#include "abstract_boards_data_access.h"

class BoardsDataAccess : public AbstractBoardsDataAccess
{
    Q_OBJECT
public:
    BoardsDataAccess(QObject *parent = nullptr);
};

#endif // BOARDSDATAACCESS_H
