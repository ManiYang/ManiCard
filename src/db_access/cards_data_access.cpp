#include "cards_data_access.h"
#include "models/node_labels.h"
#include "neo4j_http_api_client.h"
#include "utilities/async_routine.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/strings_util.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

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

    if (cardIds.isEmpty()) {
        invokeAction(callbackContext, [callback]() {
            callback(true, {});
        });
        return;
    }

    //
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

void CardsDataAccess::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(MATCH (:Card {id: $fromCardId})-[r]->(:Card {id: $toCardId})
                    WHERE type(r) = $relationshipType
                    RETURN r
                )!",
                QJsonObject {
                    {"fromCardId", relationshipId.startCardId},
                    {"toCardId", relationshipId.endCardId},
                    {"relationshipType", relationshipId.type}
                }
            },
            // callback:
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, std::nullopt);
                    return;
                }

                const auto queryResult = queryResponse.getResult().value();
                if (queryResponse.hasNetworkOrDbError()) {
                    callback(false, std::nullopt);
                    return;
                }

                if (queryResult.rowCount() == 0) {
                    callback(true, std::nullopt); // (relationship not found)
                    return;
                }

                const auto relObjectOpt = queryResult.objectValueAt(0, "r");
                if (!relObjectOpt.has_value()) {
                    callback(false, std::nullopt);
                }
                else {
                    RelationshipProperties properties;
                    properties.update(relObjectOpt.value());
                    callback(true, properties);
                }
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

void CardsDataAccess::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MATCH (n:UserSettings)
                    RETURN n.labelsList AS labels, n.relationshipTypesList AS relTypes
                )!",
                QJsonObject {}
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {QStringList(), QStringList()});
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    callback(true, {QStringList(), QStringList()});
                    return;
                }

                const auto labelsArray = result.valueAt(0, "labels").toArray();
                const auto relTypesArray = result.valueAt(0, "relTypes").toArray();

                const QStringList labelsList = toStringList(labelsArray, "");
                const QStringList relTypesList = toStringList(relTypesArray, "");

                callback(true, {labelsList, relTypesList});
            },
            callbackContext
    );
}

void CardsDataAccess::queryCustomDataQueries(
        const QSet<int> &dataQueryIds,
        std::function<void (bool, const QHash<int, CustomDataQuery> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    if (dataQueryIds.isEmpty()) {
        invokeAction(callbackContext, [callback]() {
            callback(true, {});
        });
        return;
    }

    //
    const QJsonArray dataQueryIdsArray = toJsonArray(dataQueryIds);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(MATCH (q:DataQuery)
                    WHERE q.id IN $dataQueryIds
                    RETURN q AS dataQuery
                )!",
                QJsonObject {{"dataQueryIds", dataQueryIdsArray}}
            },
            // callback:
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, {});
                    return;
                }

                //
                const auto queryResult = queryResponse.getResult().value();

                QHash<int, CustomDataQuery> result;
                bool hasError = false;
                for (int r = 0; r < queryResult.rowCount(); ++r) {
                    const auto dataQueryOpt = queryResult.objectValueAt(r, "dataQuery");
                    if (!dataQueryOpt.has_value()) {
                        hasError = true;
                        break;
                    }
                    const QJsonObject dataQueryObj = dataQueryOpt.value();

                    const QJsonValue idValue = dataQueryObj.value("id");
                    if (!idValue.isDouble()) {
                        qWarning().noquote()
                                << QString("data-query ID not found or has unexpected type");
                        hasError = true;
                        break;
                    }

                    result.insert(idValue.toInt(), CustomDataQuery::fromJson(dataQueryObj));
                }

                if (hasError)
                    callback(false, {});
                else
                    callback(true, result);
            },
            callbackContext
    );
}

void CardsDataAccess::performCustomCypherQuery(
        const QString &cypher, const QJsonObject &parameters,
        std::function<void (bool ok, const QVector<QJsonObject> &rows)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        Neo4jTransaction *transaction {nullptr};
        QVector<QJsonObject> resultRows;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // open transaction
        routine->transaction = neo4jHttpApiClient->getTransaction();
        routine->transaction->open(
                // callback:
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        routine->errorMsg = "could not open transaction";
                        context.setErrorFlag();
                    }
                },
                routine
        );
    }, routine);

    routine->addStep([routine, cypher, parameters]() {
        // perform query
        routine->transaction->query(
                QueryStatement {cypher, parameters},
                // callback
                [routine](bool ok, const QueryResponseSingleResult &queryResponse) {
                    ContinuationContext context(routine);

                    if (queryResponse.hasNetworkError) {
                        routine->errorMsg = "network error";
                        context.setErrorFlag();
                        return;
                    }

                    if (!queryResponse.dbErrors.isEmpty()) {
                        QStringList errorMessages;
                        for (const auto &dbError: queryResponse.dbErrors)
                            errorMessages << QString("(%1) %2").arg(dbError.code, dbError.message);
                        routine->errorMsg = errorMessages.join("\n\n");
                        context.setErrorFlag();
                        return;
                    }

                    if (!ok || !queryResponse.getResult().has_value()) {
                        if (ok)
                            qWarning().noquote() << "result not found while no error";
                        routine->errorMsg = "unknown error";
                        context.setErrorFlag();
                        return;
                    }

                    //
                    const auto result = queryResponse.getResult().value();
                    const QStringList columnNames = result.getColumnNames();

                    QVector<QJsonObject> rows;
                    for (int r = 0; r < result.rowCount(); ++r) {
                        QJsonObject row;
                        for (int c = 0; c < columnNames.count(); ++c)
                            row.insert(columnNames.at(c), result.valueAt(r, c));
                        rows << row;
                    }
                    routine->resultRows = rows;
                },
                routine
        );
    }, routine);

    routine->addStep([routine]() {
        // rollback transaction
        routine->transaction->rollback(
                // callback:
                [routine](bool /*ok*/) {
                    // (It's OK if the rollback failed, as the DB eventually closes the
                    // transaction without committing it.)
                    routine->nextStep();
                },
                routine
        );
    }, routine);

    routine->addStep([callback, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag)
            callback(false, { QJsonObject {{"errorMsg", routine->errorMsg}} });
        else
            callback(true, routine->resultRows);

        routine->transaction->deleteLater();
    }, callbackContext);

    routine->start();
}

void CardsDataAccess::requestNewCardId(
        std::function<void (bool, int)> callback,
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
                if (!queryResponse.getResult().has_value()) {
                    callback(false, -1);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                const int cardId = result.valueAt(0, "cardId").toInt(-1);
                if (cardId == -1) {
                    qWarning().noquote()
                            << QString("\"cardId\" value not found or has unexpected type");
                    callback(false, -1);
                    return;
                }

                callback(true, cardId);
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
            [callback, cardId](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                if (queryResponse.hasNetworkOrDbError()) {
                    callback(false);
                    return;
                }

                const auto queryResult = queryResponse.getResult().value();
                if (queryResult.isEmpty()) {
                    qWarning().noquote()
                            << QString("card %1 not found while updating card properties")
                               .arg(cardId);
                    callback(false);
                    return;
                }

                qInfo().noquote() << QString("updated properties of card %1").arg(cardId);
                callback(true);
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
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
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
                    ContinuationContext context(routine);

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
                    ContinuationContext context(routine);

                    const bool good
                            = ok && queryResponse.getResult().has_value()
                              && !queryResponse.getResult().value().isEmpty();
                    if (!good)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([routine]() {
        // 4. commit transaction
        routine->transaction->commit(
                // callback:
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                routine
        );
    }, routine);

    routine->addStep([callback, routine]() {
        // 5. (final step) call `callback` and clean up
        ContinuationContext context(routine);
        callback(!routine->errorFlag);
        routine->transaction->deleteLater();
    }, callbackContext);

    routine->start();
}

void CardsDataAccess::createRelationship(
        const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                    QString(R"!(
                        MATCH (c1:Card {id: $fromCardId})
                        MATCH (c2:Card {id: $toCardId})
                        MERGE (c1)-[r:#RelationshipType#]->(c2)
                        ON CREATE SET r._is_created = true
                        ON MATCH SET r._is_created = false
                        WITH r, r._is_created AS isCreated
                        REMOVE r._is_created
                        RETURN isCreated
                    )!")
                        .replace("#RelationshipType#", id.type),
                QJsonObject {
                        {"fromCardId", id.startCardId},
                        {"toCardId", id.endCardId}
                }
            },
            // callback
            [callback, id](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false, false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote()
                            << QString("start card %1 or end card %2 not found")
                               .arg(id.startCardId).arg(id.endCardId);
                    callback(false, false);
                    return;
                }

                const std::optional<bool> isCreated = result.boolValueAt(0, "isCreated");
                if (!isCreated.has_value()) {
                    callback(false, false);
                    return;
                }

                if (!isCreated.value()) {
                    callback(true, false);
                    return;
                }

                qInfo().noquote() << QString("created relationship %1").arg(id.toString());
                callback(true, true);
            },
            callbackContext
    );
}

void CardsDataAccess::updateUserRelationshipTypes(
        const QStringList &updatedRelTypes, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MERGE (u:UserSettings)
                    SET u.relationshipTypesList = $relTypesList
                    RETURN u
                )!",
                QJsonObject {
                        {"relTypesList", toJsonArray(updatedRelTypes)},
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote() << "result has no record";
                    callback(false);
                    return;
                }

                qInfo().noquote() << "updated userSettings.relationshipTypesList";
                callback(true);
            },
            callbackContext
    );
}

void CardsDataAccess::updateUserCardLabels(
        const QStringList &updatedCardLabels, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    neo4jHttpApiClient->queryDb(
            QueryStatement {
                R"!(
                    MERGE (u:UserSettings)
                    SET u.labelsList = $labelsList
                    RETURN u
                )!",
                QJsonObject {
                        {"labelsList", toJsonArray(updatedCardLabels)},
                }
            },
            // callback
            [callback](const QueryResponseSingleResult &queryResponse) {
                if (!queryResponse.getResult().has_value()) {
                    callback(false);
                    return;
                }

                const auto result = queryResponse.getResult().value();
                if (result.isEmpty()) {
                    qWarning().noquote() << "result has no record";
                    callback(false);
                    return;
                }

                qInfo().noquote() << "updated userSettings.labelsList";
                callback(true);
            },
            callbackContext
    );
}
