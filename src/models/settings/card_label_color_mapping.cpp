#include <QJsonArray>
#include "card_label_color_mapping.h"
#include "utilities/json_util.h"

#define APPEND_ERROR_MSG(msg) if (errorMsg != nullptr) *errorMsg += msg;

CardLabelToColorMapping::CardLabelToColorMapping()
    : AbstractWorkspaceOrBoardSetting(SettingCategory::CardLabelToColorMapping) {
}

QString CardLabelToColorMapping::toJsonStr(const QJsonDocument::JsonFormat format) const {
    QJsonArray labelsAndColorsArray;
    for (const auto &labelAndColor: qAsConst(cardLabelsAndAssociatedColors))
        labelsAndColorsArray << QJsonArray {labelAndColor.first, labelAndColor.second.name()};

    const QJsonObject obj {
        {"cardLabelsAndAssociatedColors", labelsAndColorsArray},
        {"defaultColor", defaultNodeRectColor.name()}
    };
    return QJsonDocument(obj).toJson(format).trimmed();
}

QString CardLabelToColorMapping::schema() const {
    return removeCommonIndentation(R"%(
        {
            ("cardLabelsAndAssociatedColors"): [
                ["<LabelName>", "<labelColor>"],
                ...
            ],
            ("defaultColor"): "<defaultColor>"
        }
    )%");
}

bool CardLabelToColorMapping::validate(const QString &s, QString *errorMsg) {
    return fromJsonStr(s, errorMsg).has_value();
}

bool CardLabelToColorMapping::setFromJsonStr(const QString &jsonStr) {
    const auto other = fromJsonStr(jsonStr);
    if (!other.has_value())
        return false;

    (*this) = other.value();
    return true;
}

std::optional<CardLabelToColorMapping> CardLabelToColorMapping::fromJsonStr(
        const QString &jsonStr, QString *errorMsg) {
    CardLabelToColorMapping data;
    if (errorMsg != nullptr)
        *errorMsg = "";

    QString errMsg1;
    const QJsonObject obj = parseAsJsonObject(jsonStr, &errMsg1);
    if (!errMsg1.isEmpty()) {
        APPEND_ERROR_MSG("not a valid JSON object");
        return std::nullopt;
    }

    //
    if (obj.contains("cardLabelsAndAssociatedColors")) {
        const QJsonValue v = obj.value("cardLabelsAndAssociatedColors");
        if (!v.isArray()) {
            APPEND_ERROR_MSG("[\"cardLabelsAndAssociatedColors\"] must be an array");
            return std::nullopt;
        }

        const QJsonArray labelsAndColorsArray = v.toArray();
        for (const QJsonValue &labelAndColorValue: labelsAndColorsArray) {
            if (!jsonValueIsArrayOfSize(labelAndColorValue, 2)) {
                APPEND_ERROR_MSG(
                        "[\"cardLabelsAndAssociatedColors\"][i] must be an array of size 2");
                return std::nullopt;
            }

            const QString label = labelAndColorValue[0].toString().trimmed();
            if (label.isEmpty()) {
                APPEND_ERROR_MSG("<LabelName> must be a non-empty string");
                return std::nullopt;
            }

            const QColor color = QColor(labelAndColorValue[1].toString());
            if (!color.isValid()) {
                APPEND_ERROR_MSG("<labelColor> must be a valid color");
                return std::nullopt;
            }

            data.cardLabelsAndAssociatedColors.append({label, color});
        }
    }
    if (obj.contains("defaultColor")) {
        const QColor color(obj.value("defaultColor").toString());
        if (!color.isValid()) {
            APPEND_ERROR_MSG("<defaultColor> must be a valid color");
            return std::nullopt;
        }

        data.defaultNodeRectColor = color;
    }

    return data;
}

CardLabelToColorMapping &CardLabelToColorMapping::operator =(
        const CardLabelToColorMapping &other) {
    cardLabelsAndAssociatedColors = other.cardLabelsAndAssociatedColors;
    defaultNodeRectColor = other.defaultNodeRectColor;
    return *this;
}

#undef APPEND_ERROR_MSG
