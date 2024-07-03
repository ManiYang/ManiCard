#include "relationship.h"

bool RelationshipId::connectsCard(const int cardId, int *theOtherCard) const {
    int cardId2 = -1;
    if (startCardId == cardId)
        cardId2 = endCardId;
    else if (endCardId == cardId)
        cardId2 = startCardId;

    if (theOtherCard != nullptr)
        *theOtherCard = cardId2;
    return cardId2 != -1;
}

bool RelationshipId::operator ==(const RelationshipId &other) const {
    return startCardId == other.startCardId
            && endCardId == other.endCardId
            && type == other.type;
}

QString RelationshipId::toString() const {
    return QString("(%1)-[%2]->(%3)").arg(startCardId).arg(type).arg(endCardId);
}

QJsonObject RelationshipId::toJson() const {
    return {
        {"startCardId", startCardId},
        {"endCardId", endCardId},
        {"type", type}
    };
}

RelationshipId RelationshipId::fromJson(const QJsonObject &obj) {
    return RelationshipId(
            obj.value("startCardId").toInt(),
            obj.value("endCardId").toInt(),
            obj.value("type").toString()
    );
}

//====

RelationshipProperties &RelationshipProperties::update(const QJsonObject &/*obj*/) {
    return *this;
}

QJsonObject RelationshipProperties::toJson() const {
    QJsonObject obj;
    return obj;
}

