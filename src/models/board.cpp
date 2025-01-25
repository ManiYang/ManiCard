#include <QJsonArray>
#include "board.h"
#include "utilities/json_util.h"

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}},
        {"zoomRatio", zoomRatio},
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
}

void Board::updateNodeProperties(const BoardNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(topLeftPos);
    UPDATE_PROPERTY(zoomRatio);

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

    return obj;
}

QSet<QString> BoardNodePropertiesUpdate::keys() const {
    return keySet(toJson());
}
