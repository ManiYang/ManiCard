#include <QJsonArray>
#include "board.h"
#include "utilities/json_util.h"

namespace {
QString convertRelIdToJointsDataToJsonStr(
        const QHash<RelationshipId, QVector<QPointF>> &relIdToJoints);
QHash<RelationshipId, QVector<QPointF>> getRelIdToJointsDataFromJsonStr(const QString &s);
}

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}},
        {"zoomRatio", zoomRatio},
        {"cardPropertiesToShow", cardPropertiesToShow.toJsonStr(QJsonDocument::Compact)},
        {"relIdToJoints", convertRelIdToJointsDataToJsonStr(relIdToJoints)}
    };
}

void Board::updateNodeProperties(const QJsonObject &obj) {
    if (const auto v = obj.value("name"); !v.isUndefined())
        name = v.toString();

    if (const auto v = obj.value("topLeftPos"); !v.isUndefined()) {
        const QJsonArray array = v.toArray();
        if (array.count() == 2)
            topLeftPos = QPointF(array.at(0).toDouble(), array.at(1).toDouble());
        else
            topLeftPos = QPointF(0, 0);
    }

    if (const auto v = obj.value("zoomRatio"); !v.isUndefined()) {
        zoomRatio = v.toDouble(1.0);
    }

    if (const QJsonValue v = obj.value("cardPropertiesToShow"); v.isString()) {
        QString errorMsg;
        const auto dataOpt = CardPropertiesToShow::fromJsonStr(v.toString(), &errorMsg);
        if (dataOpt.has_value()) {
            cardPropertiesToShow = dataOpt.value();
        }
        else {
            qWarning().noquote()
                    << QString("could not parse the string as a CardPropertiesToShow");
            qWarning().noquote() << QString("  | string -- %1").arg(v.toString());
            qWarning().noquote() << QString("  | error msg -- %1").arg(errorMsg);
            cardPropertiesToShow = CardPropertiesToShow();
        }
    }

    if (const auto v = obj.value("relIdToJoints"); !v.isUndefined()) {
        relIdToJoints = getRelIdToJointsDataFromJsonStr(v.toString());
    }
}

void Board::updateNodeProperties(const BoardNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(topLeftPos);
    UPDATE_PROPERTY(zoomRatio);
    UPDATE_PROPERTY(cardPropertiesToShow);
    UPDATE_PROPERTY(relIdToJoints);

#undef UPDATE_PROPERTY
}

int Board::findParentGroupBoxOfGroupBox(const int groupBoxId) const {
    for (auto it = groupBoxIdToData.constBegin(); it != groupBoxIdToData.constEnd(); ++it) {
        if (it.value().childGroupBoxes.contains(groupBoxId))
            return it.key();
    }
    return -1;
}

int Board::findParentGroupBoxOfCard(const int cardId) const {
    for (auto it = groupBoxIdToData.constBegin(); it != groupBoxIdToData.constEnd(); ++it) {
        if (it.value().childCards.contains(cardId))
            return it.key();
    }
    return -1;
}

bool Board::isGroupBoxADescendantOfGroupBox(const int groupBoxId1, const int groupBoxId2) const {
    Q_ASSERT(groupBoxId1 != -1 && groupBoxId2 != -1);

    if (groupBoxId1 == groupBoxId2)
        return false;

    int id = groupBoxId1;
    while (true) {
        const int parent = findParentGroupBoxOfGroupBox(id);
        if (parent == -1)
            return false;
        else if (parent == groupBoxId2)
            return true;
        id = parent;
    }
}

bool Board::hasSettingBoxFor(
        const SettingTargetType targetType, const SettingCategory category) const {
    for (const SettingBoxData data: qAsConst(settingBoxesData)) {
        if (data.targetType == targetType && data.category == category)
            return true;
    }
    return false;
}

void Board::updateSettingBoxData(
        const SettingTargetType targetType, const SettingCategory category,
        const SettingBoxDataUpdate &update) {
    for (auto it = settingBoxesData.begin(); it != settingBoxesData.end(); ++it) {
        SettingBoxData &data = *it;
        if (data.targetType == targetType && data.category == category) {
            data.update(update);
            return;
        }
    }
}

void Board::removeSettingBoxData(
        const SettingTargetType targetType, const SettingCategory category) {
    for (auto it = settingBoxesData.begin(); it != settingBoxesData.end(); /*nothing*/) {
        SettingBoxData &data = *it;
        if (data.targetType == targetType && data.category == category)
            it = settingBoxesData.erase(it);
        else
            ++it;
    }
}

//====

QJsonObject BoardNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (topLeftPos.has_value())
        obj.insert("topLeftPos", QJsonArray{topLeftPos.value().x(), topLeftPos.value().y()});

    if (zoomRatio.has_value())
        obj.insert("zoomRatio", zoomRatio.value());

    if (cardPropertiesToShow.has_value()) {
        obj.insert(
                "cardPropertiesToShow",
                cardPropertiesToShow.value().toJsonStr(QJsonDocument::Compact));
    }

    if (relIdToJoints.has_value()) {
        obj.insert(
                "relIdToJoints",
                convertRelIdToJointsDataToJsonStr(relIdToJoints.value()));
    }

    return obj;
}

QSet<QString> BoardNodePropertiesUpdate::keys() const {
    return keySet(toJson());
}

//====

namespace {
QString convertRelIdToJointsDataToJsonStr(
        const QHash<RelationshipId, QVector<QPointF>> &relIdToJoints) {
    QJsonArray array;
    for (auto it = relIdToJoints.constBegin(); it != relIdToJoints.constEnd(); ++it) {
        const RelationshipId &relId = it.key();
        const QVector<QPointF> &joints = it.value();

        QJsonArray jointsArray;
        for (const QPointF &p: joints)
            jointsArray << QJsonArray {p.x(), p.y()};

        array << QJsonArray {relId.toJson(), jointsArray};
    }

    constexpr bool compact = true;
    return printJson(array, compact);
}

QHash<RelationshipId, QVector<QPointF>> getRelIdToJointsDataFromJsonStr(const QString &s) {
    QHash<RelationshipId, QVector<QPointF>> result;

    const QJsonArray array = parseAsJsonArray(s);
    for (const QJsonValue &relItem: array) {
        if (!jsonValueIsArrayOfSize(relItem, 2))
            continue;
        if (!relItem[0].isObject())
            continue;

        //
        const auto relId = RelationshipId::fromJson(relItem[0].toObject());
        if (relId.startCardId == -1)
            continue;

        //
        if (!relItem[1].isArray())
            continue;
        const QJsonArray jointsArray = relItem[1].toArray();

        QVector<QPointF> joints;
        for (const QJsonValue &jointItem: jointsArray) {
            if (!jsonValueIsArrayOfSize(jointItem, 2))
                continue;
            if (!jointItem[0].isDouble() || !jointItem[1].isDouble())
                continue;
            joints << QPointF(jointItem[0].toDouble(), jointItem[1].toDouble());
        }

        //
        result.insert(relId, joints);
    }

    return result;
}
} // namespace
