#ifndef CARDS_DATA_ACCESS_H
#define CARDS_DATA_ACCESS_H

#include "abstract_cards_data_access.h"

class Neo4jHttpApiClient;

class CardsDataAccess : public AbstractCardsDataAccess
{
public:
    explicit CardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient);

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

    // ==== write ====

    void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext) override;

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext) override;

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) override;

private:
    Neo4jHttpApiClient *neo4jHttpApiClient;
};

#endif // CARDS_DATA_ACCESS_H
