#ifndef RELATIONSHIP_H
#define RELATIONSHIP_H

#include <QJsonObject>
#include <QString>
#include "utilities/hash.h"

//!
//! In Neo4j, for specific combination of (start-node, end-node, relationship-type), there can be
//! only at most one relationship. Thus the combination can be used to identify a relationship.
//!
struct RelationshipId
{
    RelationshipId(const int startCardId, const int endCardId, const QString &type)
        : startCardId(startCardId), endCardId(endCardId), type(type) {}

    int startCardId;
    int endCardId;
    QString type;

    //!
    //! \param cardId
    //! \param theOtherCard: will be the card ID with which \e cardId is connected by this
    //!                      relationship, or -1 if this relationship does not connect \e cardId.
    //! \return whether this relationship connects \e cardId from/to another card
    //!
    bool connectsCard(const int cardId, int *theOtherCard = nullptr) const;

    bool operator == (const RelationshipId &other) const;

    QString toString() const;

    QJsonObject toJson() const;
    static RelationshipId fromJson(const QJsonObject &obj);
};

inline uint qHash(const RelationshipId &id, uint seed)
{
    hashCombine(seed, id.startCardId);
    hashCombine(seed, id.endCardId);
    hashCombine(seed, id.type);
    return seed;
}


struct RelationshipProperties
{
    RelationshipProperties &update(const QJsonObject &obj);
    QJsonObject toJson() const;
};

#endif // RELATIONSHIP_H
