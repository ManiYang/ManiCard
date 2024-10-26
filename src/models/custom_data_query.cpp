#include <QRegularExpression>
#include <QSet>
#include "custom_data_query.h"
#include "utilities/json_util.h"
#include "utilities/strings_util.h"

void CustomDataQuery::update(const CustomDataQueryUpdate &update) {
#define UPDATE_ITEM(item) \
    if (update.item.has_value()) this->item = update.item.value();

    UPDATE_ITEM(title);
    UPDATE_ITEM(queryCypher);
    UPDATE_ITEM(queryParameters);

#undef UPDATE_ITEM
}

QJsonObject CustomDataQuery::toJson() const {
    return QJsonObject {
        {"title", title},
        {"queryCypher", queryCypher},
        {"queryParameters", printJson(queryParameters)}
    };
}

CustomDataQuery CustomDataQuery::fromJson(const QJsonObject &obj) {
    CustomDataQuery dataQuery;

    if (const auto v = obj.value("title"); !v.isUndefined())
        dataQuery.title = v.toString();

    if (const auto v = obj.value("queryCypher"); !v.isUndefined())
        dataQuery.queryCypher = v.toString();

    if (const auto v = obj.value("queryParameters"); !v.isUndefined()) {
        const QString queryParametersStr = v.toString();
        dataQuery.queryParameters = parseAsJsonObject(queryParametersStr);
    }

    return dataQuery;
}

bool CustomDataQuery::validateCypher(const QString queryCypher, QString *msg) {
    const QString cypher = queryCypher.toUpper();

    static const QRegularExpression re("\\s+");
    const QStringList words = cypher.split(re, QString::SkipEmptyParts);
    const QSet<QString> wordsSet(words.constBegin(), words.constEnd());

    //
    if (!wordsSet.contains("RETURN")) {
        if (msg != nullptr)
            *msg = "RETURN not found";
        return false;
    }

    //
    static const QSet<QString> disallowedWords {
        "CREATE", "MERGE", "LOAD", "DETACH", "DELETE", "SET", "REMOVE"};

    if (const auto intersection = wordsSet & disallowedWords; !intersection.isEmpty()) {
        if (msg != nullptr) {
            constexpr bool sort = true;
            *msg = QString("Disallowed keyword(s): %1")
                   .arg(joinStringSet(intersection, ", ", sort));
        }
        return false;
    }

    //
    return true;
}

void CustomDataQueryUpdate::mergeWith(const CustomDataQueryUpdate &other) {
#define UPDATE_ITEM(item) \
    if (other.item.has_value()) item = other.item;

    UPDATE_ITEM(title);
    UPDATE_ITEM(queryCypher);
    UPDATE_ITEM(queryParameters);
#undef UPDATE_ITEM
}

QJsonObject CustomDataQueryUpdate::toJson() const {
    QJsonObject obj;

    if (title.has_value())
        obj.insert("title", title.value());
    if (queryCypher.has_value())
        obj.insert("queryCypher", queryCypher.value());
    if (queryParameters.has_value())
        obj.insert("queryParameters", printJson(queryParameters.value()));

    return obj;
}
