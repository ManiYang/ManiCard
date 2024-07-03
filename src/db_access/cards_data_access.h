#ifndef CARDS_DATA_ACCESS_H
#define CARDS_DATA_ACCESS_H

#include "abstract_cards_data_access.h"

class Neo4jHttpApiClient;

class CardsDataAccess : public AbstractCardsDataAccess
{
    Q_OBJECT
public:
    explicit CardsDataAccess(
            Neo4jHttpApiClient *neo4jHttpApiClient, QObject *parent = nullptr);

    // ==== read ====

    void queryCards(
                const QSet<int> &cardIds,
                std::function<void (bool ok, const QHash<int, Card> &)> callback,
                QPointer<QObject> callbackContext) override;

    void traverseFromCard(
            const int startCardId,
            std::function<void (bool ok, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool ok, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // CARDS_DATA_ACCESS_H
