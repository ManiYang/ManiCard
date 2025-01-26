#include "card_properties_to_show.h"
#include "utilities/json_util.h"

#define SET_ERROR_MSG(msg) if (errorMsg != nullptr) *errorMsg = msg;

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

//====

CardPropertiesToShow::CardPropertiesToShow()
    : AbstractWorkspaceOrBoardSetting(SettingCategory::CardPropertiesToShow) {
}

QString CardPropertiesToShow::toJsonStr(const QJsonDocument::JsonFormat jsonPrintFormat) const {
    QJsonArray array;
    for (const auto &[propertyName, format]: qAsConst(propertyNamesAndDisplayFormats))
        array << QJsonObject {{propertyName, format.toJson()}};

    return QJsonDocument(array).toJson(jsonPrintFormat);
}

QString CardPropertiesToShow::schema() const {
    return QString(R"%(
    [
      {
        "<propertyName>": "<defaultStringWhenExists>~The value is $"
      },
      {
        "<propertyName>": {
          ("case <value>"): "<displayStringForSpecificValue>",
          ...,
          ("default"): "<defaultStringWhenExists>~The value is $",
          ("ifNotExists"): "<displayStringWhenNotExists>",
          ("hideLabel"): <hideLabel?>,
          ("addQuotesForString"): <addQuotesForString?>
        }
      }
      ...
    ]
    )%").trimmed();
}

bool CardPropertiesToShow::validate(const QString &s, QString *errorMsg) {
    return fromJsonStr(s, errorMsg).has_value();
}

bool CardPropertiesToShow::setFromJsonStr(const QString &jsonStr) {
    const auto other = fromJsonStr(jsonStr);
    if (!other.has_value())
        return false;

    propertyNamesAndDisplayFormats = other.value().propertyNamesAndDisplayFormats;
    return true;
}

std::optional<CardPropertiesToShow> CardPropertiesToShow::fromJsonStr(
        const QString &jsonStr, QString *errorMsg) {
    CardPropertiesToShow result;
    SET_ERROR_MSG("");

    QString errMsg1;
    const QJsonArray array = parseAsJsonArray(jsonStr, &errMsg1);
    if (!errMsg1.isEmpty()) {
        SET_ERROR_MSG("not a valid JSON array");
        return std::nullopt;
    }

    for (const QJsonValue &v: array) {
        if (!v.isObject()) {
            SET_ERROR_MSG("setting[i] must be an object");
            return std::nullopt;
        }
        const auto propertyObj = v.toObject();

        if (propertyObj.count() != 1) {
            SET_ERROR_MSG("setting[i] must be an object with exactly one key (the property name)");
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

        result.propertyNamesAndDisplayFormats.append({propertyName, formatOpt.value()});
    }

    return result;
}

#undef SET_ERROR_MSG
