#ifndef RELATIONSHIPSBUNDLE_H
#define RELATIONSHIPSBUNDLE_H

#include <QString>

struct RelationshipsBundle
{
    enum class Direction {IntoGroup, OutFromGroup};

    int groupBoxId {-1};
    int externalCardId {-1};
    QString relationshipType;
    Direction direction {Direction::IntoGroup};
};

#endif // RELATIONSHIPSBUNDLE_H
