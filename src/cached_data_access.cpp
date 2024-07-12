#include <QDateTime>
#include "cached_data_access.h"
#include "db_access/queued_db_access.h"
#include "file_access/unsaved_update_records_file.h"
#include "utilities/async_routine.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/maps_util.h"

CachedDataAccess::CachedDataAccess(
        QueuedDbAccess *queuedDbAccess_,
        std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
        QObject *parent)
            : QObject(parent)
            , queuedDbAccess(queuedDbAccess_)
            , unsavedUpdateRecordsFile(unsavedUpdateRecordsFile_) {
}

void CachedDataAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        // variables used by the steps of the routine:
        QHash<int, Card> cardsResult;
        bool dbQueryOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. get the parts that are already cached
    for (const int id: cardIds) {
        if (cache.cards.contains(id))
            routine->cardsResult.insert(id, cache.cards.value(id));
    }

    // 2. query DB for the other parts
    //   + if successful: update cache
    //   + if failed: whole process fails
    const QSet<int> cardsToQuery = cardIds - keySet(routine->cardsResult);

    routine->addStep([this, cardsToQuery, routine]() {
        queuedDbAccess->queryCards(
                cardsToQuery,
                // callback:
                [this, routine](bool queryOk, const QHash<int, Card> &cardsFromDb) {
                    routine->dbQueryOk = queryOk;
                    if (queryOk) {
                        mergeWith(routine->cardsResult, cardsFromDb);
                        // update cache
                        mergeWith(cache.cards, cardsFromDb);
                    }
                    routine->nextStep();
                },
                this
        );
    }, this);

    routine->addStep([routine, callback]() {
        if (routine->dbQueryOk)
            callback(true, routine->cardsResult);
        else
            callback(false, {});
        routine->nextStep();
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    queuedDbAccess->requestNewCardId(
            // callback
            [callback](std::optional<int> cardId) {
                callback(cardId);
            },
            callbackContext
    );
}

void CachedDataAccess::createNewCardWithId(
        const int cardId, const Card &card, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        // variables used by the steps of the routine:
        bool writeDbOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    routine->addStep([this, routine, cardId, card]() {
        auto context = routine->continuationContext();

        if (cache.cards.contains(cardId)) {
            qWarning().noquote()
                    << QString("card with ID %1 already exists in cache").arg(cardId);
            context.setErrorFlag();
            return;
        }

        cache.cards.insert(cardId, card);
    }, this);

    // 2. write DB
    routine->addStep([this, routine, cardId, card]() {
        queuedDbAccess->createNewCardWithId(
                cardId, card,
                // callback
                [routine](bool ok) {
                    auto context = routine->continuationContext();
                    routine->writeDbOk = ok;
                    // if ok = false, errorFlag is set in the next step
                },
                this
        );
    }, this);

    // 3. if write DB failed, add to unsaved updates
    routine->addStep([this, routine, cardId, card]() {
        auto context = routine->continuationContext();
        if (!routine->writeDbOk) {
            context.setErrorFlag();

            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "createNewCardWithId";
            const QString updateDetails = printJson(QJsonObject {
                {"cardId", cardId},
                {"labels", toJsonArray(card.getLabels())},
                {"cardProperties", card.getPropertiesJson()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    // 4. (final step) call callback
    routine->addStep([routine, callback]() {
        auto context = routine->continuationContext();
        callback(!routine->errorFlag);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        // variables used by the steps of the routine:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    if (cache.cards.contains(cardId))
        cache.cards[cardId].updateProperties(cardPropertiesUpdate);

    // 2. write DB
    routine->addStep([this, routine, cardId, cardPropertiesUpdate]() {
        queuedDbAccess->updateCardProperties(
                cardId, cardPropertiesUpdate,
                // callback
                [routine](bool ok) {
                    routine->dbWriteOk = ok;
                    routine->nextStep();
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, cardId, cardPropertiesUpdate]() {
        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateCardProperties";
            const QString updateDetails = printJson(QJsonObject {
                {"cardId", cardId},
                {"propertiesUpdate", cardPropertiesUpdate.toJson()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
        routine->nextStep();
    }, this);

    //
    routine->addStep([routine, callback]() {
        callback(routine->dbWriteOk);
        routine->nextStep();
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
        // variables used by the steps of the routine:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    if (cache.cards.contains(cardId))
        cache.cards[cardId].setLabels(updatedLabels);

    // 2. write DB
    routine->addStep([this, routine, cardId, updatedLabels]() {
        queuedDbAccess->updateCardLabels(
                cardId, updatedLabels,
                // callback
                [routine](bool ok) {
                    routine->dbWriteOk = ok;
                    routine->nextStep();
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, cardId, updatedLabels]() {
        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateCardLabels";
            const QString updateDetails = printJson(QJsonObject {
                {"cardId", cardId},
                {"updatedLabels", toJsonArray(updatedLabels)}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([routine, callback]() {
        callback(routine->dbWriteOk);
        routine->nextStep();
    }, callbackContext);

    routine->start();
}
