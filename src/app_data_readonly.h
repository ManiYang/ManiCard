#ifndef APP_DATA_READONLY_H
#define APP_DATA_READONLY_H

#include "models/board.h"
#include "models/boards_list_properties.h"
#include "models/card.h"
#include "models/relationship.h"

class AppDataReadonly : public QObject
{
    Q_OBJECT
public:
    explicit AppDataReadonly(QObject *parent = nullptr);

    // ==== persisted data ====

    virtual void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    virtual void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    using StringListPair = std::pair<QStringList, QStringList>;
    virtual void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual std::optional<QSize> getMainWindowSize() = 0;

    // ==== non-persisted independent data ====

    virtual int getHighlightedCardId() const = 0; // can return -1
};

#endif // APP_DATA_READONLY_H
