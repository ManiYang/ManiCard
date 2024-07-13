#include "cards_data_access.h"
#include "models/node_labels.h"
#include "neo4j_http_api_client.h"
#include "utilities/async_routine.h"
#include "utilities/json_util.h"
#include "utilities/strings_util.h"

using QueryStatement = Neo4jHttpApiClient::QueryStatement;
using QueryResponseSingleResult = Neo4jHttpApiClient::QueryResponseSingleResult;

CardsDataAccess::CardsDataAccess(Neo4jHttpApiClient *neo4jHttpApiClient)
    : AbstractCardsDataAccess()
    , neo4jHttpApiClient(neo4jHttpApiClient)
{}

void CardsDataAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    const QJsonArray cardIdsArray = toJsonArray(cardIds);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(MATCH (c:Card)
                    WHERE c.id IN $cardIds
                    RETURN c AS card, labels(c) AS labels
                )!",
                QJsonObject {{"cardIds", cardIdsArray}}
            },
            // callback:
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<int, Card> cardsResult;
                bool hasError = false;

                if (queryResponse.hasNetworkOrDbError())
                    hasError = true;

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
            QueryStatement {
                R"!(MATCH (c0:Card {id: $startCardId})
                    RETURN c0 AS card, labels(c0) AS labels
                    UNION
                    MATCH (c0:Card {id: $startCardId})-[r*]->(c:Card)
                    RETURN c AS card, labels(c) AS labels
                )!",
                QJsonObject {{"startCardId", startCardId}}
            },
            // callback:
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<int, Card> cardsResult;
                bool hasError = false;

                if (queryResponse.hasNetworkOrDbError())
                    hasError = true;

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
            QueryStatement {
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
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<RelId, RelProperties> result;
                bool hasError = false;

                if (queryResponse.hasNetworkOrDbError())
                    hasError = true;

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

void CardsDataAccess::requestNewCardId(
        std::function<void (std::optional<int>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:LastUsedId {itemType: 'Card'})
                    SET n.value = n.value + 1
                    RETURN n.value AS cardId
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value())
                {
                    callback(std::nullopt);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                const int cardId = result.valueAt(0, "cardId").toInt(-1);
                if (cardId == -1) {
                    qWarning().noquote()
                            << QString("\"cardId\" value not found or has unexpected type");
                    callback(std::nullopt);
                    return;
                }

                callback(cardId);
            },
            callbackContext
    );
}

void CardsDataAccess::createNewCardWithId(
        const int cardId, const Card &card,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    QString labelsInSetClause = "";
    if (!card.getLabels().isEmpty())
        labelsInSetClause = QString("c:%1,").arg(joinStringSet(card.getLabels(), ":"));

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                QString(R"!(
                    MERGE (c:Card {id: $cardId})
                    ON CREATE
                        SET #LabelsInSetClause# c += $propertiesMap, c._is_created_ = true
                    ON MATCH
                        SET c._is_created_ = false
                    WITH c, c._is_created_ AS isCreated
                    REMOVE c._is_created_
                    RETURN isCreated
                )!")
                    .replace("#LabelsInSetClause#", labelsInSetClause),
                QJsonObject {
                    {"cardId", cardId},
                    {"propertiesMap", card.getPropertiesJson()}
                }
            },
            // callback
            [callback, cardId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                QJsonValue isCreatedValue = result.valueAt(0, "isCreated");
                if (!isCreatedValue.isBool()) {
                    qWarning().noquote()
                            << QString("\"isCreated\" value not found or has unexpected type");
                    callback(false);
                    return;
                }

                if (isCreatedValue.toBool()) {
                    qInfo().noquote() << QString("created card with ID %1").arg(cardId);
                    callback(true);
                }
                else {
                    qWarning().noquote()
                            << QString("card with ID %1 already exists").arg(cardId);
                    callback(false);
                }
            },
            callbackContext
    );
}

void CardsDataAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(MATCH (c:Card {id: $cardId})
                    SET c += $propertiesMap
                    RETURN c.id
                )!",
                QJsonObject {
                    {"cardId", cardId},
                    {"propertiesMap", cardPropertiesUpdate.toJson()}
                }
            },
            // callback:
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                bool hasError = false;

                if (queryResponse.hasNetworkOrDbError())
                    hasError = true;

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    qWarning().noquote() << "card not found";
                    hasError = true;
                }

                callback(!hasError);
            },
            callbackContext
    );
}

void CardsDataAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        // variables used by the steps of the routine:
        Neo4jTransaction *transaction {nullptr};
        QSet<QString> oldLabels; // other than "Card"
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback:
                [routine](bool ok) {
                    auto context = routine->continuationContext().setErrorFlagWhen(!ok);
                },
                routine
        );
    }, routine);

    routine->addStep([cardId, routine]() {
        // 2. query card labels
        Q_ASSERT(routine->transaction->canQuery());

        routine->transaction->query(
                QueryStatement {
                    R"!(
                        MATCH (c:Card {id: $cardId})
                        SET c._temp_ = 1
                        RETURN labels(c) AS labels
                    )!",
                    QJsonObject {{"cardId", cardId}}
                },
                // callback:
                [routine](bool ok, const QueryResponseSingleResult &queryResponse) {
                    auto context = routine->continuationContext();

                    if (!ok || !queryResponse.getResult().has_value()) {
                        context.setErrorFlag();
                        return;
                    }

                    const auto queryResult = queryResponse.getResult().value();
                    const QJsonValue labelsValue
                            = queryResult.valueAt(0, "labels"); // can be Undefined
                    if (!labelsValue.isArray()) {
                        qWarning().noquote()
                                << QString("card not found or \"labels\" value has unexpected type");
                        context.setErrorFlag();
                        return;
                    }

                    const auto labels = toStringList(labelsValue.toArray(), "");
                    for (const QString &label: labels) {
                        if (label != NodeLabel::card && !label.isEmpty())
                            routine->oldLabels << label;
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, cardId, updatedLabels]() {
        // 3. add/remove card labels
        const QSet<QString> labelsToAdd = updatedLabels - routine->oldLabels;
        const QSet<QString> labelsToRemove = routine->oldLabels - updatedLabels;

        const QString setLabelsClause = labelsToAdd.isEmpty()
                ? ""
                : QString("SET c:%1").arg(
                      QStringList(labelsToAdd.cbegin(), labelsToAdd.cend()).join(":"));
        const QString removeLabelsClause = labelsToRemove.isEmpty()
                ? ""
                : QString("REMOVE c:%1").arg(
                      QStringList(labelsToRemove.cbegin(), labelsToRemove.cend()).join(":"));

        routine->transaction->query(
                QueryStatement {
                    QString(R"!(
                        MATCH (c:Card {id: $cardId})
                        #set-labels-clause#
                        #remove-labels-clause#
                        REMOVE c._temp_
                        RETURN c.id
                    )!")
                        .replace("#set-labels-clause#", setLabelsClause)
                        .replace("#remove-labels-clause#", removeLabelsClause),
                    QJsonObject {{"cardId", cardId}}
                },
                // callback:
                [routine](bool ok, const QueryResponseSingleResult &queryResponse) {
                    auto context = routine->continuationContext();

                    const bool good
                            = ok && queryResponse.getResult().has_value()
                              && !queryResponse.getResult().value().isEmpty();
                    context.setErrorFlagWhen(!good);
                },
                routine
        );
    }, routine);

    routine->addStep([routine]() {
        // 4. commit transaction
        routine->transaction->commit(
                // callback:
                [routine](bool ok) {
                    auto context = routine->continuationContext().setErrorFlagWhen(!ok);
                },
                routine
        );
    }, routine);

    routine->addStep([callback, routine]() {
        // 5. (final step) call `callback` and clean up
        auto context = routine->continuationContext();
        callback(!routine->errorFlag);
        routine->transaction->deleteLater();
    }, callbackContext);

    routine->start();
}
