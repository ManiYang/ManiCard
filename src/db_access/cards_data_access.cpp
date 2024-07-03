#include "cards_data_access.h"
#include "neo4j_http_api_client.h"
#include "utilities/json_util.h"

CardsDataAccess::CardsDataAccess(
        Neo4jHttpApiClient *neo4jHttpApiClient, QObject *parent)
    : AbstractCardsDataAccess(parent)
    , neo4jHttpApiClient(neo4jHttpApiClient)
{}

void CardsDataAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    const QJsonArray cardIdsArray = toJsonArray(cardIds);

    neo4jHttpApiClient->queryDb(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (c:Card)
                    WHERE c.id IN $cardIds
                    RETURN c AS card, labels(c) AS labels
                )!",
                QJsonObject {{"cardIds", cardIdsArray}}
            },
            // callback:
            [callback](const Neo4jHttpApiClient::QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<int, Card> cardsResult;
                bool hasError = false;

                for (int r = 0; r < queryResult.rowCount(); ++r) {
                    const QJsonValue cardProperties = queryResult.valueAt(r, "card");
                    const QJsonValue cardlabels = queryResult.valueAt(r, "labels");

                    if (!cardProperties.isObject() || !cardlabels.isArray()) {
                        if (!hasError) {
                            hasError = true;
                            qWarning().noquote()
                                    << QString("value not found or has unexpected type");
                        }
                        continue;
                    }

                    const QJsonValue idValue = cardProperties["id"];
                    if (!idValue.isDouble()) {
                        if (!hasError) {
                            hasError = true;
                            qWarning().noquote()
                                    << QString("card ID not found or has unexpected type");
                        }
                        continue;
                    }

                    cardsResult.insert(
                            idValue.toInt(),
                            Card()
                                .addLabels(toStringList(cardlabels.toArray(), ""))
                                .updateProperties(cardProperties.toObject())
                    );
                }
                callback(!hasError, cardsResult);
            },
            callbackContext
    );
}

void CardsDataAccess::traverseFromCard(
        const int startCardId, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (c0:Card {id: $startCardId})
                    RETURN c0 AS card, labels(c0) AS labels
                    UNION
                    MATCH (c0:Card {id: $startCardId})-[r*]->(c:Card)
                    RETURN c AS card, labels(c) AS labels
                )!",
                QJsonObject {{"startCardId", startCardId}}
            },
            // callback:
            [callback](const Neo4jHttpApiClient::QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<int, Card> cardsResult;
                bool hasError = false;

                for (int r = 0; r < queryResult.rowCount(); ++r) {
                    const QJsonValue properties = queryResult.valueAt(r, "card");
                    const QJsonValue labels = queryResult.valueAt(r, "labels");

                    if (!properties.isObject() || !labels.isArray()) {
                        if (!hasError) {
                            hasError = true;
                            qWarning().noquote()
                                    << QString("value not found or has unexpected type");
                        }
                        continue;
                    }

                    const QJsonValue idValue = properties["id"];
                    if (!idValue.isDouble()) {
                        if (!hasError) {
                            hasError = true;
                            qWarning().noquote()
                                    << QString("card ID not found or has unexpected type");
                        }
                        continue;
                    }

                    cardsResult.insert(
                            idValue.toInt(),
                            Card()
                                .addLabels(toStringList(labels.toArray(), ""))
                                .updateProperties(properties.toObject())
                    );
                }
                callback(!hasError, cardsResult);
            },
            callbackContext
    );
}

void CardsDataAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    neo4jHttpApiClient->queryDb(
            Neo4jHttpApiClient::QueryStatement {
                R"!(MATCH (c0:Card)-[r]->(c1:Card)
                    WHERE c0.id IN $cardIdList
                    RETURN c0.id AS startCardId, c1.id AS endCardId, r AS rel, type(r) AS relType
                    UNION
                    MATCH (c0:Card)-[r]->(c1:Card)
                    WHERE c1.id IN $cardIdList
                    RETURN c0.id AS startCardId, c1.id AS endCardId, r AS rel, type(r) AS relType
                )!",
                QJsonObject {{"cardIdList", toJsonArray(cardIds)}}
            },
            // callback:
            [callback](const Neo4jHttpApiClient::QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<RelId, RelProperties> result;
                bool hasError = false;

                for (int r = 0; r < queryResult.rowCount(); ++r) {
                    const QJsonValue startCardId = queryResult.valueAt(r, "startCardId");
                    const QJsonValue endCardId = queryResult.valueAt(r, "endCardId");
                    const QJsonValue relProperties = queryResult.valueAt(r, "rel");
                    const QJsonValue relType = queryResult.valueAt(r, "relType");

                    if (!startCardId.isDouble() || !endCardId.isDouble()
                            || !relProperties.isObject() || !relType.isString()) {
                        if (!hasError)                         {
                            hasError = true;
                            qWarning().noquote()
                                    << QString("value not found or has unexpected type");
                        }
                        continue;
                    }

                    result.insert(
                            RelId(startCardId.toInt(), endCardId.toInt(), relType.toString()),
                            RelProperties().update(relProperties.toObject())
                    );
                }
                callback(!hasError, result);
            },
            callbackContext
    );
}
