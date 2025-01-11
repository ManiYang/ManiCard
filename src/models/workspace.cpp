#include "utilities/json_util.h"
#include "workspace.h"

namespace {
QString convertCardLabelsAndAssociatedColorsToJsonStr(
        const QVector<Workspace::LabelAndColor> &cardLabelsAndAssociatedColors);
QVector<Workspace::LabelAndColor> parseAsCardLabelsAndAssociatedColors(const QString &s);
}

QJsonObject Workspace::getNodePropertiesJson() const {
    return QJsonObject {
        {"name", name},
        {"boardsOrdering", toJsonArray(boardsOrdering)},
        {"lastOpenedBoardId", lastOpenedBoardId},
        {"defaultNodeRectColor", defaultNodeRectColor.name(QColor::HexRgb)},
        {"cardLabelsAndAssociatedColors",
            convertCardLabelsAndAssociatedColorsToJsonStr(cardLabelsAndAssociatedColors)}
    };
}

void Workspace::updateNodeProperties(const QJsonObject &obj) {
    if (const QJsonValue v = obj.value("name"); !v.isUndefined())
        name = v.toString();
    if (const QJsonValue v = obj.value("boardsOrdering"); !v.isUndefined())
        boardsOrdering = toIntVector(v.toArray(), -1);
    if (const QJsonValue v = obj.value("lastOpenedBoardId"); !v.isUndefined())
        lastOpenedBoardId = v.toInt();
    if (const QJsonValue v = obj.value("defaultNodeRectColor"); !v.isUndefined()) {
        const QColor color(v.toString());
        defaultNodeRectColor = color.isValid() ? color : defaultNodeRectColorFallback;
    }
    if (const QJsonValue v = obj.value("cardLabelsAndAssociatedColors"); !v.isUndefined())
        cardLabelsAndAssociatedColors = parseAsCardLabelsAndAssociatedColors(v.toString());
}

void Workspace::updateNodeProperties(const WorkspaceNodePropertiesUpdate &update) {
#define UPDATE_PROPERTY(member) \
        if (update.member.has_value()) \
            this->member = update.member.value();

    UPDATE_PROPERTY(name);
    UPDATE_PROPERTY(boardsOrdering);
    UPDATE_PROPERTY(lastOpenedBoardId);
    UPDATE_PROPERTY(defaultNodeRectColor);
    UPDATE_PROPERTY(cardLabelsAndAssociatedColors);

#undef UPDATE_PROPERTY
}

QJsonObject WorkspaceNodePropertiesUpdate::toJson() const {
    QJsonObject obj;

    if (name.has_value())
        obj.insert("name", name.value());

    if (boardsOrdering.has_value())
        obj.insert("boardsOrdering", toJsonArray(boardsOrdering.value()));

    if (lastOpenedBoardId.has_value())
        obj.insert("lastOpenedBoardId", lastOpenedBoardId.value());

    if (defaultNodeRectColor.has_value())
        obj.insert("defaultNodeRectColor", defaultNodeRectColor.value().name(QColor::HexRgb));

    if (cardLabelsAndAssociatedColors.has_value()) {
        obj.insert(
                "cardLabelsAndAssociatedColors",
                convertCardLabelsAndAssociatedColorsToJsonStr(cardLabelsAndAssociatedColors.value())
        );
    }

    return obj;
}

//======

namespace {
QString convertCardLabelsAndAssociatedColorsToJsonStr(
        const QVector<Workspace::LabelAndColor> &cardLabelsAndAssociatedColors) {
    QJsonArray array;
    for (const auto &[label, color]: cardLabelsAndAssociatedColors)
        array << QJsonArray {label, color.name(QColor::HexRgb)};
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

QVector<Workspace::LabelAndColor> parseAsCardLabelsAndAssociatedColors(const QString &s) {
    const auto doc = QJsonDocument::fromJson(s.toUtf8());
    if (!doc.isArray())
        return {};
    const QJsonArray array = doc.array();

    QVector<Workspace::LabelAndColor> result;
    for (const QJsonValue &item: array) {
        if (!item.isArray())
            continue;
        const auto itemArray = item.toArray();
        if (itemArray.count() != 2)
            continue;

        const QString label = itemArray.at(0).toString();
        const QColor color = QColor(itemArray.at(1).toString());

        result << std::make_pair(label, color);
    }
    return result;
}
} // namespace;
