#include <QJsonArray>
#include "board.h"
#include "utilities/json_util.h"

namespace {
QJsonArray convertToJsonArray(const QVector<Board::LabelAndColor> &vector);
}

QJsonObject Board::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"topLeftPos", QJsonArray {topLeftPos.x(), topLeftPos.y()}},
        {"zoomRatio", zoomRatio},
        {"defaultNodeRectColor", defaultNodeRectColor.name(QColor::HexRgb)},
        {"cardLabelsAndAssociatedColors",
            printJson(convertToJsonArray(cardLabelsAndAssociatedColors), true)}
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

    if (const auto v = obj.value("defaultNodeRectColor"); !v.isUndefined()) {
        const QColor color(v.toString());
        if (color.isValid())
            defaultNodeRectColor = color;
        else
            defaultNodeRectColor = QColor(170, 170, 170);
    }

    if (const auto v = obj.value("cardLabelsAndAssociatedColors"); !v.isUndefined()) {
        cardLabelsAndAssociatedColors.clear();

        const auto doc = QJsonDocument::fromJson(v.toString().toUtf8());
        const QJsonArray array = doc.isNull() ? QJsonArray() : doc.array();
        for (const QJsonValue &item: array) {
            const auto subarray = item.toArray();
            if (subarray.count() != 2)
                continue;

            const auto label = subarray.at(0).toString();
            const QColor color(subarray.at(1).toString());
            if (!label.isEmpty() && color.isValid())
                cardLabelsAndAssociatedColors << std::make_pair(label, color);
        }
    }
}

void Board::updateNodeProperties(const BoardNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(topLeftPos);
    UPDATE_PROPERTY(zoomRatio);
    UPDATE_PROPERTY(defaultNodeRectColor);
    UPDATE_PROPERTY(cardLabelsAndAssociatedColors);

#undef UPDATE_PROPERTY
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

    if (defaultNodeRectColor.has_value()) {
        obj.insert("defaultNodeRectColor", defaultNodeRectColor.value().name(QColor::HexRgb));
    }

    if (cardLabelsAndAssociatedColors.has_value()) {
        obj.insert(
                "cardLabelsAndAssociatedColors",
                printJson(convertToJsonArray(cardLabelsAndAssociatedColors.value()), true));
    }

    return obj;
}

QSet<QString> BoardNodePropertiesUpdate::keys() const {
    return keySet(toJson());
}

//====

namespace {
QJsonArray convertToJsonArray(const QVector<Board::LabelAndColor> &vector) {
    QJsonArray result;
    for (const auto &item: vector)
        result << QJsonArray {item.first, item.second.name(QColor::HexRgb)};
    return result;
}
}
