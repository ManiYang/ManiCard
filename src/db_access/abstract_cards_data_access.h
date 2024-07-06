#ifndef ABSTRACT_CARDS_DATA_ACCESS_H
#define ABSTRACT_CARDS_DATA_ACCESS_H

#include <QObject>
#include <QPointer>
#include "models/card.h"
#include "models/relationship.h"

class AbstractCardsDataAccessReadOnly
{
public:
    explicit AbstractCardsDataAccessReadOnly();

    //!
    //! \param cardIds
    //! \param callback: parameter \e cards contains only cards that are found
    //! \param callbackContext
    //!
    virtual void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool ok, const QHash<int, Card> &cards)> callback,
            QPointer<QObject> callbackContext) = 0;

    //!
    //! Get all cards that are reachable via a path starting from \e startCardId.
    //! \e startCardId itself is included in the result.
    //!
    virtual void traverseFromCard(
            const int startCardId,
            std::function<void (bool ok, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    //!
    //! Get all relationships that start or end at one of \e cardIds.
    //!
    virtual void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool ok, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) = 0;
private:

};

class AbstractCardsDataAccess : public AbstractCardsDataAccessReadOnly
{
public:
    explicit AbstractCardsDataAccess();

};

#endif // ABSTRACT_CARDS_DATA_ACCESS_H