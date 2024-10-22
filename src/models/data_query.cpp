#include <QRegularExpression>
#include <QSet>
#include "data_query.h"
#include "utilities/strings_util.h"

bool DataQuery::validateCypher(const QString queryCypher, QString *msg) {
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
