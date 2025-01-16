#ifndef RELATIONSHIPSBUNDLE_H
#define RELATIONSHIPSBUNDLE_H

#include <QString>
#include "utilities/hash.h"

struct RelationshipsBundle
{
    enum class Direction {IntoGroup, OutFromGroup};

    int groupBoxId {-1};
    int externalCardId {-1};
    QString relationshipType;
    Direction direction {Direction::IntoGroup};

    //
    bool operator == (const RelationshipsBundle &other) const;

    QString toString() const;
};

inline uint qHash(const RelationshipsBundle &b, uint seed) {
    hashCombine(seed, b.groupBoxId);
    hashCombine(seed, b.externalCardId);
    hashCombine(seed, b.relationshipType);
    hashCombine(seed, static_cast<int>(b.direction));
    return seed;
}

#endif // RELATIONSHIPSBUNDLE_H
