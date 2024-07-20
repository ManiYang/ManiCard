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
};

class AbstractCardsDataAccess : public AbstractCardsDataAccessReadOnly
{
public:
    explicit AbstractCardsDataAccess();

    virtual void requestNewCardId(
            std::function<void (bool ok, int cardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    //!
    //! Card with ID \e cardId must not already exist. This operation is atomic.
    //!
    virtual void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The card must exist. This operation is atomic.
    //!
    virtual void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    //!
    //! The card must exist. This operation is atomic and idempotent.
    //!
    virtual void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext) = 0;

    // ==== relationships ====

    //!
    //! The start/end cards must already exist (it's an error otherwise).
    //! It's not an error if the relationship already exists (in this case `created` will be
    //! false in the arguments of callback).
    //! This operation is atomic.
    //!
    virtual void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext) = 0;
};

#endif // ABSTRACT_CARDS_DATA_ACCESS_H
