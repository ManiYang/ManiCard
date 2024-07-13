#include <QTimer>
#include "queued_db_access.h"
#include "utilities/functor.h"

QueuedDbAccess::QueuedDbAccess(
        std::shared_ptr<AbstractBoardsDataAccess> boardsDataAccess,
        std::shared_ptr<AbstractCardsDataAccess> cardsDataAccess, QObject *parent)
    : QObject(parent)
    , boardsDataAccess(boardsDataAccess)
    , cardsDataAccess(cardsDataAccess)
{}

void QueuedDbAccess::clearErrorFlag() {
    errorFlag = false;
}

void QueuedDbAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = true; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false, {}); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->queryCards( // <-- method
                cardIds, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok, const QHash<int, Card> &result) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok, result); // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };

    addToQueue(func);
}

void QueuedDbAccess::traverseFromCard(
        const int startCardId, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = true; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false, {}); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->traverseFromCard( // <-- method
                startCardId, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok, const QHash<int, Card> &result) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok, result); // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };

    addToQueue(func);
}

void QueuedDbAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = true; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false, {}); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->queryRelationshipsFromToCards( // <-- method
                cardIds, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok, const QHash<RelId, RelProperties> &result) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok, result);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = true; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(std::nullopt); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->requestNewCardId( // <-- method
                // no params // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (std::optional<int> result) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(result);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(result.has_value(), isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::createNewCardWithId(
        const int cardId, const Card &card,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = false; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->createNewCardWithId( // <-- method
                cardId, card, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = false; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->updateCardProperties( // <-- method
                cardId, cardPropertiesUpdate, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = false; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->cardsDataAccess->updateCardLabels( // <-- method
                cardId, updatedLabels, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    constexpr bool isReadOnlyAccess = true; // <--

    auto func = [=, thisPtr=QPointer(this)](const bool failDirectly) {
        if (failDirectly) {
            invokeAction(callbackContext, [callback]() {
                callback(false, {}); // <-- callback params
            });
            if (thisPtr)
                thisPtr->onResponse(false, isReadOnlyAccess);
            return;
        }

        if (thisPtr.isNull())
            return;
        thisPtr->boardsDataAccess->getBoardData( // <-- method
                boardId, // <-- input params
                // callback:
                [thisPtr, callback, callbackContext]
                        (bool ok, std::optional<Board> board) { // <-- callback params
                    invokeAction(callbackContext, [=]() {
                        callback(ok, board);  // <-- callback params
                    });
                    if (thisPtr)
                        thisPtr->onResponse(ok, isReadOnlyAccess);
                },
                thisPtr.data()
        );
    };
    addToQueue(func);
}

void QueuedDbAccess::addToQueue(std::function<void (const bool)> func) {
    const bool failDirectly = errorFlag;
    queue << Task {func, failDirectly};

    if (!isWaitingResponse) {
        dequeueAndInvoke();
        isWaitingResponse = true;
    }
}

void QueuedDbAccess::onResponse(const bool ok, const bool isReadOnlyAccess) {
    if (!isReadOnlyAccess && !ok) {
        if (!errorFlag) {
            errorFlag = true;
            // let all remaining task fail directly (without data access)
            for (Task &task: queue)
                task.toFailDirectly = true;
        }
    }

    if (!queue.isEmpty())
        dequeueAndInvoke();
    else
        isWaitingResponse = false;
}

void QueuedDbAccess::dequeueAndInvoke() {
    Q_ASSERT(!queue.isEmpty());
    auto task = queue.dequeue();

    // add `func` to the event queue (rather than call it directly) to prevent deep call stack
    QTimer::singleShot(0, this, [task]() {
        task.func(task.toFailDirectly);
    });
}
