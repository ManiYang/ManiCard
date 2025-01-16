#include "relationships_bundle.h"

bool RelationshipsBundle::operator ==(const RelationshipsBundle &other) const {
    return groupBoxId == other.groupBoxId
            && externalCardId == other.externalCardId
            && relationshipType == other.relationshipType
            && direction == other.direction;
}

QString RelationshipsBundle::toString() const {
    if (direction == Direction::IntoGroup) {
        return QString("(group %1)<-[%2]--(card %3)")
                .arg(groupBoxId).arg(relationshipType).arg(externalCardId);
    }
    else {
        return QString("(group %1)--[%2]->(card %3)")
                .arg(groupBoxId).arg(relationshipType).arg(externalCardId);
    }
}
