#ifndef BOARDS_LIST_PROPERTIES_H
#define BOARDS_LIST_PROPERTIES_H

#include <QJsonObject>
#include <QVector>

struct BoardsListPropertiesUpdate;

struct BoardsListProperties
{
    int lastOpenedBoard {-1}; // can be a non-existing board ID
    QVector<int> boardsOrdering; // may not contain all existing board IDs, and may contain
                                 // non-existing board IDs

    //
    void update(const BoardsListPropertiesUpdate &propertiesUpdate);
    void update(const QJsonObject &propertiesUpdate);
};


struct BoardsListPropertiesUpdate
{
    std::optional<int> lastOpenedBoard;
    std::optional<QVector<int>> boardsOrdering;

    QJsonObject toJson() const;
    QSet<QString> keys() const;
};

#endif // BOARDS_LIST_PROPERTIES_H
