#ifndef QUEUED_DB_ACCESS_H
#define QUEUED_DB_ACCESS_H

#include <functional>
#include <memory>
#include <QObject>
#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"

//!
//! A proxy of \c BoardsDataAccess & \c CardsDataAccess. The requests are queued and handled in
//! sequence (sends next request only when current one gets response).
//!
//! When a non-read-only operation failed, all remaining requests in the queue will fail directly
//! (without actually being performed). Before the error flag is cleared, any new request will also
//! fail directly.
//!
class QueuedDbAccess
        : public QObject, public AbstractBoardsDataAccess, public AbstractCardsDataAccess
{
    Q_OBJECT
public:
    QueuedDbAccess(
            std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
            std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess,
            QObject *parent = nullptr);

    void clearErrorFlag();

    // ==== AbstractCardsDataAccess interface ====

    // read operations

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void traverseFromCard(
            const int startCardId,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

    void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext) override;

    // write operations

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext) override;

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;



    // ==== AbstractBoardsDataAccess interface ====

    // read operations

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
                const int boardId,
                std::function<void (bool ok, std::optional<Board> board)> callback,
                QPointer<QObject> callbackContext) override;

    // write operations

    void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext) override;

    void createNewBoardWithId(
            const int boardId, const Board &board,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeBoard(
            const int boardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void removeNodeRect(
            const int boardId, const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

private:
    std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess;
    std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess;

    struct Task
    {
        std::function<void (const bool failDirectly)> func;
        bool toFailDirectly;
    };
    QQueue<Task> queue;

    bool isWaitingResponse {false};
    bool errorFlag {false}; // set when a request failed, unset by clearErrorFlag()

    void addToQueue(std::function<void (const bool failDirectly)> func);
    void onResponse(const bool ok, const bool isReadOnlyAccess = false);
    void dequeueAndInvoke();
};

#endif // QUEUED_DB_ACCESS_H
