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

    // write operations

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;



    // ==== AbstractBoardsDataAccess interface ====

    // read operations

    // write operations


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
