#ifndef QUEUED_DB_ACCESS_H
#define QUEUED_DB_ACCESS_H

#include <functional>
#include <memory>
#include <QObject>
#include "abstract_boards_data_access.h"
#include "abstract_cards_data_access.h"

class QueuedDbAccess
        : public AbstractBoardsDataAccess, public AbstractCardsDataAccess, public QObject
{
    Q_OBJECT
public:
    QueuedDbAccess(
            std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
            std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess,
            QObject *parent = nullptr);

    void clearErrorFlag();

    // ==== AbstractCardsDataAccess interface ====

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

    // ==== AbstractBoardsDataAccess interface ====


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
    void onResponse(const bool ok);
    void dequeueAndInvoke();
};

#endif // QUEUED_DB_ACCESS_H
