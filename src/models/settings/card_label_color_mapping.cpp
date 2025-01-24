#include <QJsonArray>
#include "card_label_color_mapping.h"
#include "utilities/json_util.h"

CardLabelToColorMapping::CardLabelToColorMapping()
    : AbstractWorkspaceOrBoardSetting(SettingCategory::CardLabelToColorMapping) {
}

QJsonObject CardLabelToColorMapping::toJson() const {
    QJsonArray labelsAndColorsArray;
    for (const auto &labelAndColor: qAsConst(cardLabelsAndAssociatedColors))
        labelsAndColorsArray << QJsonArray {labelAndColor.first, labelAndColor.second.name()};

    return {
        {"cardLabelsAndAssociatedColors", labelsAndColorsArray},
        {"defaultColor", defaultNodeRectColor.name()}
    };
}

void CardLabelToColorMapping::setFromJson(const QJsonObject &obj) {
    cardLabelsAndAssociatedColors.clear();
    defaultNodeRectColor = defaultNodeRectColorFallback;

    //
    const QJsonArray labelsAndColorsArray = obj.value("cardLabelsAndAssociatedColors").toArray();
    for (const QJsonValue &labelAndColor: labelsAndColorsArray) {
        if (jsonValueIsArrayOfSize(labelAndColor, 2)) {
            cardLabelsAndAssociatedColors.append(
                    LabelAndColor {labelAndColor[0].toString(), QColor(labelAndColor[1].toString())}
            );
        }
    }

    if (const auto v = obj.value("defaultColor"); v.isString())
        defaultNodeRectColor = v.toString();
}

CardLabelToColorMapping &CardLabelToColorMapping::operator =(
        const CardLabelToColorMapping &other) {
    cardLabelsAndAssociatedColors = other.cardLabelsAndAssociatedColors;
    defaultNodeRectColor = other.defaultNodeRectColor;
    return *this;
}
