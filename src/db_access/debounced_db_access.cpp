#include <QDebug>
#include "debounced_db_access.h"
#include "utilities/action_debouncer.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"

DebouncedDbAccess::DebouncedDbAccess(
        std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
        std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess, QObject *parent)
            : QObject(parent)
            , boardsDataAccess(boardsDataAccess)
            , cardsDataAccess(cardsDataAccess) {
}

void DebouncedDbAccess::finalize() {
    closeDebounceSession();
}

void DebouncedDbAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->queryCards(cardIds, callback, callbackContext);
}

void DebouncedDbAccess::traverseFromCard(
        const int startCardId, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->traverseFromCard(startCardId, callback, callbackContext);
}

void DebouncedDbAccess::updateCardPropertiesDebounced(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate) {
    const DebounceKey debounceKey {
        DebounceDataCategory::CardProperties,
        QJsonObject {{"cardId", cardId}}
    };

    if (currentDebounceSession.has_value()) {
        if (currentDebounceSession.value().key() != debounceKey)
            closeDebounceSession();
    }

    //
    if (!currentDebounceSession.has_value()) {
        // set cumulated update data
        cumulatedUpdateData.cardPropertiesUpdate = cardPropertiesUpdate;

        // create debounce session
        constexpr int separationMsec = 1500;
        currentDebounceSession = DebounceSession(
                debounceKey,
                separationMsec,
                // action:
                [this, cardId]() {
                    cardsDataAccess->updateCardProperties(
                            cardId,
                            cumulatedUpdateData.cardPropertiesUpdate,
                            // callback
                            [](bool ok) {
                                // ..........
                            },
                            this
                    );

                    // clear cumulated update data
                    cumulatedUpdateData.cardPropertiesUpdate = CardPropertiesUpdate();
                }
        );
        qInfo().noquote()
                << QString("entered debounce session %1")
                   .arg(currentDebounceSession.value().printKey());
    }
    else { // (already in the debounce session)
        Q_ASSERT(currentDebounceSession.value().key() == debounceKey);

        // cumulate update data
        cumulatedUpdateData.cardPropertiesUpdate.mergeWith(cardPropertiesUpdate);

        //
        currentDebounceSession.value().tryAct();
    }
}

QString DebouncedDbAccess::debounceDataCategoryName(const DebounceDataCategory category) {
    switch (category) {
    case DebounceDataCategory::CardProperties: return "CardProperties";
    }
    return "";
}

void DebouncedDbAccess::closeDebounceSession() {
    const QString sessionStr = currentDebounceSession.value().printKey();

    currentDebounceSession = std::nullopt;
            // If currentDebounceSession originally has value, this calls destructor of the
            // wrapped DebounceSession.

    qInfo().noquote() << QString("closed debounce session %1").arg(sessionStr);
}

//====

DebouncedDbAccess::DebounceSession::DebounceSession(
        const DebounceKey &debounceKey, const int separationMsec,
        std::function<void ()> action)
            : category(debounceKey.first)
            , dataKeys(debounceKey.second) {
    debouncer = new ActionDebouncer(separationMsec, ActionDebouncer::Option::Delay, action);
}

DebouncedDbAccess::DebounceSession::~DebounceSession() {
    if (debouncer == nullptr)
        return;

    // close the session
    if (debouncer->hasDelayed())
        debouncer->actNow();
    debouncer->deleteLater();
    debouncer = nullptr;
}

std::pair<DebouncedDbAccess::DebounceDataCategory, QJsonObject>
DebouncedDbAccess::DebounceSession::key() const {
    return {category, dataKeys};
}

void DebouncedDbAccess::DebounceSession::tryAct() {
    Q_ASSERT(debouncer != nullptr);
    debouncer->tryAct();
}

QString DebouncedDbAccess::DebounceSession::printKey() const {
    const QString categoryStr = debounceDataCategoryName(category);

    constexpr bool compact = true;
    const QString dataKeysStr = printJson(dataKeys, compact);

    return QString("(%1, %2)").arg(categoryStr, dataKeysStr);
}
