#include "card_properties_to_show.h"
#include "utilities/json_util.h"

#define SET_ERROR_MSG(msg) if (errorMsg != nullptr) *errorMsg = msg;

QString ValueDisplayFormat::getValueDisplayText(const QJsonValue &value) const {
    Q_ASSERT(!value.isUndefined());

    QString printedValue;
    if (value.isString()) {
        if (addQuotesForString)
            printedValue = QString("\"%1\"").arg(value.toString());
        else
            printedValue = value.toString();
    }
    else if (value.isBool()) {
        printedValue = value.toBool() ? "true" : "false";
    }
    else if (value.isDouble()) {
        if (jsonValueIsInt(value))
            printedValue = QString::number(value.toInt());
        else
            printedValue = QString::number(value.toDouble());
    }
    else if (value.isArray()) {
        constexpr bool compact = true;
        printedValue = printJson(value.toArray(), compact);
    }
    else if (value.isObject()) {
        constexpr bool compact = true;
        printedValue = printJson(value.toObject(), compact);
    }
    else if (value.isNull()) {
        printedValue = "null";
    }
    else { // (should not happen)
        printedValue = "";
    }

    //
    QString displayString;
    if (caseValueToString.contains(printedValue))
        displayString = caseValueToString.value(printedValue);
    else
        displayString = defaultStringIfExists;

    // split with "$$"
    QStringList splitted = displayString.split("$$");

    // replace '$' by `printedValue`
    for (int i = 0; i < splitted.count(); ++i)
         splitted[i].replace(QChar('$'), printedValue);

    // join with "$"
    return splitted.join("$");
}

QJsonObject ValueDisplayFormat::toJson() const {
    QJsonObject obj;

    for (auto it = caseValueToString.constBegin(); it != caseValueToString.constEnd(); ++it) {
        obj.insert(
                QString("case %1").arg(it.key()),
                it.value()
        );
    }

    obj.insert("default", defaultStringIfExists);

    if (stringIfNotExists.has_value())
        obj.insert("ifNotExists", stringIfNotExists.value());

    if (hideLabel)
        obj.insert("hideLabel", true);

    if (addQuotesForString)
        obj.insert("addQuotesForString", true);

    return obj;
}

std::optional<ValueDisplayFormat> ValueDisplayFormat::fromJson(
        const QJsonValue &v, QString *errorMsg) {
    ValueDisplayFormat format;
    SET_ERROR_MSG("");

    //
    if (v.isString()) {
        const QString defaultString = v.toString();
        if (defaultString.isEmpty()) {
            SET_ERROR_MSG("<defaultStringWhenExists> cannot be empty");
            return std::nullopt;
        }
        format.defaultStringIfExists = defaultString;

        return format;
    }

    //
    if (v.isObject()) {
        const QJsonObject obj = v.toObject();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            const QString key = it.key();
            const QJsonValue value = it.value();

            if (key.startsWith("case ")) {
                if (!value.isString()) {
                    SET_ERROR_MSG(QString("value for key \"%1\" must be a string").arg(key));
                    return std::nullopt;
                }
                const QString caseValue = key.mid(5);
                format.caseValueToString.insert(caseValue, value.toString());
            }
            else if (key == "default") {
                if (!value.isString()) {
                    SET_ERROR_MSG(QString("value for key \"%1\" must be a string").arg(key));
                    return std::nullopt;
                }
                if (value.toString().isEmpty()) {
                    SET_ERROR_MSG(QString("<defaultStringWhenExists> cannot be empty").arg(key));
                    return std::nullopt;
                }
                format.defaultStringIfExists = value.toString();
            }
            else if (key == "ifNotExists") {
                if (!value.isString()) {
                    SET_ERROR_MSG(QString("value for key \"%1\" must be a string").arg(key));
                    return std::nullopt;
                }
                format.stringIfNotExists = value.toString();
            }
            else if (key == "hideLabel") {
                if (!value.isBool()) {
                    SET_ERROR_MSG(QString("value for key \"%1\" must be a Boolean").arg(key));
                    return std::nullopt;
                }
                format.hideLabel = value.toBool();
            }
            else if (key == "addQuotesForString") {
                if (!value.isBool()) {
                    SET_ERROR_MSG(QString("value for key \"%1\" must be a Boolean").arg(key));
                    return std::nullopt;
                }
                format.addQuotesForString = value.toBool();
            }
            else {
                SET_ERROR_MSG(QString("unrecognized key \"%1\"").arg(key));
                return std::nullopt;
            }
        }
        return format;
    }

    //
    SET_ERROR_MSG("value for key <propertyName> must be a string or an object");
    return std::nullopt;
}

//======

CardPropertiesToShow::CardPropertiesToShow()
    : AbstractWorkspaceOrBoardSetting(SettingCategory::CardPropertiesToShow) {
}

CardPropertiesToShow::PropertiesAndDisplayFormats CardPropertiesToShow::getPropertiesToShow(
        const QSet<QString> &cardLabels) const {
    // find first label in `cardLabelsOrdering` that `cardLabels` contains
    auto it = std::find_if(
            cardLabelsOrdering.constBegin(), cardLabelsOrdering.constEnd(),
            [cardLabels](const QString &label) {
                return cardLabels.contains(label);
            }
    );

    //
    if (it == cardLabelsOrdering.constEnd())
        return {};
    else
        return cardLabelToSetting.value(*it);
}

void CardPropertiesToShow::updateWith(const CardPropertiesToShow &other) {
    const QHash<QString, PropertiesAndDisplayFormats> originalCardLabelToSetting
            = cardLabelToSetting;

    //
    QVector<QString> labelsInNewSetting;
    QHash<QString, PropertiesAndDisplayFormats> newCardLabelToSetting;

    for (const QString &label: other.cardLabelsOrdering) {
        const auto propertiesAndDisplayFormatsUpdate = other.cardLabelToSetting.value(label);

        PropertiesAndDisplayFormats newPropertiesAndDisplayFormats;
        if (originalCardLabelToSetting.contains(label)) {
            // `label` is in both the original and the update
            newPropertiesAndDisplayFormats = merge(
                    originalCardLabelToSetting.value(label), propertiesAndDisplayFormatsUpdate);
        }
        else {
            // `label` is only in the update
            newPropertiesAndDisplayFormats = propertiesAndDisplayFormatsUpdate;
        }

        newCardLabelToSetting.insert(label, newPropertiesAndDisplayFormats);
        labelsInNewSetting << label;
    }

    // add the labels that are only in the original (to the end)
    for (const QString &label: cardLabelsOrdering) {
        if (!newCardLabelToSetting.contains(label)) {
            labelsInNewSetting << label;
            newCardLabelToSetting.insert(label, originalCardLabelToSetting.value(label));
        }
    }

    //
    cardLabelToSetting = newCardLabelToSetting;
    cardLabelsOrdering = labelsInNewSetting;
}

QString CardPropertiesToShow::toJsonStr(const QJsonDocument::JsonFormat jsonPrintFormat) const {
    QJsonArray labelsArray;

    for (const QString &labelName: qAsConst(cardLabelsOrdering)) {
        const auto propertiesAndFormats = cardLabelToSetting.value(labelName);

        QJsonArray propertiesArray;
        for (const auto &[propertyName, format]: propertiesAndFormats)
            propertiesArray << QJsonObject {{propertyName, format.toJson()}};

        labelsArray << QJsonObject {{labelName, propertiesArray}};
    }

    return QJsonDocument(labelsArray).toJson(jsonPrintFormat).trimmed();
}

QString CardPropertiesToShow::schema() const {
    return removeCommonIndentation(R"%(
    [
      { "<LabelName>" : [
          {
            "<propertyName>": "<defaultStringWhenExists>~The value is $"
          },
          { "<propertyName>": {
              ("case <value>"): "<displayStringForSpecificValue>",
              ...,
              ("default"): "<defaultStringWhenExists>~The value is $",
              ("ifNotExists"): "<displayStringWhenNotExists>",
              ("hideLabel"): <hideLabel?>,
              ("addQuotesForString"): <addQuotesForString?>
          }}
          ...
      ]},
      ...
    ]
    )%");
}

bool CardPropertiesToShow::validate(const QString &s, QString *errorMsg) {
    return fromJsonStr(s, errorMsg).has_value();
}

bool CardPropertiesToShow::setFromJsonStr(const QString &jsonStr) {
    const auto other = fromJsonStr(jsonStr);
    if (!other.has_value())
        return false;

    (*this) = other.value();
    return true;
}

std::optional<CardPropertiesToShow> CardPropertiesToShow::fromJsonStr(
        const QString &jsonStr, QString *errorMsg) {
    CardPropertiesToShow result;
    SET_ERROR_MSG("");

    QString errMsg1;
    const QJsonArray labelsArray = parseAsJsonArray(jsonStr, &errMsg1);
    if (!errMsg1.isEmpty()) {
        SET_ERROR_MSG("not a valid JSON array");
        return std::nullopt;
    }

    for (const QJsonValue &labelValue: labelsArray) {
        if (!labelValue.isObject()) {
            SET_ERROR_MSG("setting[i] must be an object");
            return std::nullopt;
        }
        const QJsonObject labelObj = labelValue.toObject();

        if (labelObj.count() != 1) {
            SET_ERROR_MSG("setting[i] must be an object with exactly one key (the label name)");
            return std::nullopt;
        }
        const QString labelName = labelObj.constBegin().key();

        if (!labelObj.constBegin().value().isArray()) {
            SET_ERROR_MSG("value of key <LabelName> must be an array");
            return std::nullopt;
        }
        const auto propertiesArray = labelObj.constBegin().value().toArray();

        PropertiesAndDisplayFormats propertiesAndDisplayFormats;
        for (const QJsonValue &propertyValue: propertiesArray) {
            if (!propertyValue.isObject()) {
                SET_ERROR_MSG("setting[i][<LabelName>][j] must be an object");
                return std::nullopt;
            }
            const auto propertyObj = propertyValue.toObject();

            if (propertyObj.count() != 1) {
                SET_ERROR_MSG("setting[i][<LabelName>][j] must be an object with "
                              "exactly one key (the property name)");
                return std::nullopt;
            }
            const QString propertyName = propertyObj.constBegin().key();

            QString errMsg1;
            const auto formatOpt = ValueDisplayFormat::fromJson(
                    propertyObj.constBegin().value(), &errMsg1);
            if (!formatOpt.has_value()) {
                SET_ERROR_MSG(errMsg1);
                return std::nullopt;
            }

            propertiesAndDisplayFormats.append({propertyName, formatOpt.value()});
        }

        result.cardLabelToSetting.insert(labelName, propertiesAndDisplayFormats);
        result.cardLabelsOrdering << labelName;
    }

    return result;
}

CardPropertiesToShow &CardPropertiesToShow::operator =(const CardPropertiesToShow &other) {
    cardLabelToSetting = other.cardLabelToSetting;
    cardLabelsOrdering = other.cardLabelsOrdering;
    return *this;
}

CardPropertiesToShow::PropertiesAndDisplayFormats CardPropertiesToShow::merge(
        const PropertiesAndDisplayFormats &vec1, const PropertiesAndDisplayFormats &vec2) {
    PropertiesAndDisplayFormats result;
    QSet<QString> propertiesInResult;

    // add properties that are in `vec2`
    for (const auto &[property, format2]: vec2) {
        result.append({property, format2});
        propertiesInResult << property;
    }

    // add properties that are only in `vec1`
    for (const auto &[property, format1]: vec1) {
        if (!propertiesInResult.contains(property)) {
            result.append({property, format1});
        }
    }

    //
    return result;
}

#undef SET_ERROR_MSG
