#include <QRegularExpression>
#include <QSet>
#include "custom_data_query.h"
#include "utilities/strings_util.h"

CustomDataQuery CustomDataQuery::fromJson(const QJsonObject &obj) {
    CustomDataQuery dataQuery;

    if (const auto v = obj.value("title"); !v.isUndefined())
        dataQuery.title = v.toString();

    if (const auto v = obj.value("queryCypher"); !v.isUndefined())
        dataQuery.queryCypher = v.toString();

    if (const auto v = obj.value("queryParameters"); !v.isUndefined())
        dataQuery.queryParameters = v.toObject();

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
