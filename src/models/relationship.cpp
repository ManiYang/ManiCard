#include <QRegularExpression>
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

QString RelationshipId::toStringRepr() const {
    return QString("(%1)-[%2]->(%3)").arg(startCardId).arg(type).arg(endCardId);
}

RelationshipId RelationshipId::fromStringRepr(const QString &s) {
    static const QRegularExpression re(R"%(^\((\d+)\)-\[(\w+)\]->\((\d+)\)$)%");

    const QString errorMsg = QString("could not parse \"%1\" as a RelationshipId").arg(s);

    const auto m = re.match(s);
    if (!m.hasMatch()) {
        qWarning().noquote() << errorMsg;
        return RelationshipId(-1, -1, "");
    }

    Q_ASSERT(m.lastCapturedIndex() == 3);
    bool ok;
    const int startCardId = m.captured(1).toInt(&ok);
    if (!ok) {
        qWarning().noquote() << errorMsg;
        return RelationshipId(-1, -1, "");
    }

    const QString relType = m.captured(2);

    const int endCardId = m.captured(3).toInt(&ok);
    if (!ok) {
        qWarning().noquote() << errorMsg;
        return RelationshipId(-1, -1, "");
    }

    return RelationshipId(startCardId, endCardId, relType);
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
            obj.value("startCardId").toInt(-1),
            obj.value("endCardId").toInt(-1),
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

