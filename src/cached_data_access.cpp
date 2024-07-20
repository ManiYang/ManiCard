#include <QDateTime>
#include <QReadLocker>
#include <QWriteLocker>
#include "cached_data_access.h"
#include "db_access/queued_db_access.h"
#include "file_access/local_settings_file.h"
#include "file_access/unsaved_update_records_file.h"
#include "utilities/async_routine.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/maps_util.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

CachedDataAccess::CachedDataAccess(
        QueuedDbAccess *queuedDbAccess_, std::shared_ptr<LocalSettingsFile> localSettingsFile_,
        std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
        QObject *parent)
            : QObject(parent)
            , queuedDbAccess(queuedDbAccess_)
            , localSettingsFile(localSettingsFile_)
            , unsavedUpdateRecordsFile(unsavedUpdateRecordsFile_) {
}

bool CachedDataAccess::hasWriteRequestInProgress() const {
    bool result;
    {
        QReadLocker locker(&lockForwriteRequestsInProgress);
        result = !writeRequestsInProgress.isEmpty();
    }
    return result;
}

void CachedDataAccess::queryCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

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

void CachedDataAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    queuedDbAccess->queryRelationshipsFromToCards(
            cardIds,
            // callback
            [this, callback, callbackContext](bool ok, const QHash<RelId, RelProperties> &rels) {
                if (!ok) {
                    invokeAction(callbackContext, [callback]() {
                        callback(false, {});
                    });
                    return;
                }

                // update cache
                mergeWith(cache.relationships, rels);

                //
                invokeAction(callbackContext, [callback, rels]() {
                    callback(true, rels);
                });
            },
            this
    );
}

void CachedDataAccess::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    queuedDbAccess->requestNewCardId(
            // callback
            [callback](bool ok, int cardId) {
                callback(ok ? cardId : std::optional<int>());
            },
            callbackContext
    );
}

void CachedDataAccess::getBoardIdsAndNames(
        std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    queuedDbAccess->getBoardIdsAndNames(
            // callback
            [callback](bool ok, const QHash<int, QString> &idToName) {
                callback(ok, idToName);
            },
            callbackContext
    );
}

void CachedDataAccess::getBoardsListProperties(
        std::function<void (bool, BoardsListProperties properties)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        BoardsListProperties properties;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get boards ordering from DB
        queuedDbAccess->getBoardsListProperties(
                // callback
                [routine](bool ok, BoardsListProperties properties) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                    else
                        routine->properties = properties;
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // get last-opened board from local settings file
        ContinuationContext context(routine);

        const auto [ok, boardIdOpt] = localSettingsFile->readLastOpenedBoardId();
        if (!ok) {
            context.setErrorFlag();
        }
        else {
            if (boardIdOpt.has_value())
                routine->properties.lastOpenedBoard = boardIdOpt.value();
        }
    }, this);

    routine->addStep([routine, callback]() {
        // (final step)
        ContinuationContext context(routine);
        if (routine->errorFlag)
            callback(false, {});
        else
            callback(true, routine->properties);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    // 1. get the parts that are already cached
    if (cache.boards.contains(boardId)) {
        const auto board = cache.boards.value(boardId);
        invokeAction(callbackContext, [callback, board]() {
            callback(true, board);
        });
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool queryDbOk;
        std::optional<Board> board; // from DB
        bool readFileOk;
        std::optional<QPointF> topLeftPos; // from settings file

        std::optional<Board> result;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 2. query DB
    routine->addStep([this, routine, boardId]() {
        queuedDbAccess->getBoardData(
                boardId,
                // callback
                [routine](bool ok, std::optional<Board> board) {
                    ContinuationContext context(routine);

                    if (ok)
                        routine->board = board;
                    routine->queryDbOk = ok;
                },
                this
        );
    }, this);

    // 3. get topLeftPos from local settings file
    routine->addStep([this, routine, boardId]() {
        ContinuationContext context(routine);

        const auto [ok, topLeftPosOpt] = localSettingsFile->readTopLeftPosOfBoard(boardId);
        if (ok)
            routine->topLeftPos = topLeftPosOpt;
        routine->readFileOk = ok;
    }, this);

    // 4. set routine->result & update cache
    routine->addStep([this, routine, boardId]() {
        ContinuationContext context(routine);

        if (!routine->queryDbOk || !routine->readFileOk) {
            context.setErrorFlag();
            return;
        }

        if (routine->board.has_value()) {
            routine->result = routine->board;
            cache.boards.insert(boardId, routine->board.value());

            if (routine->topLeftPos.has_value()) {
                routine->result.value().topLeftPos = routine->topLeftPos.value();
                cache.boards[boardId].topLeftPos = routine->topLeftPos.value();
            }
        }
    }, this);

    // 5. (final step)
    routine->addStep([routine, callback]() {
         ContinuationContext context(routine);
         callback(!routine->errorFlag, routine->result);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::requestNewBoardId(
        std::function<void (std::optional<int>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    queuedDbAccess->requestNewBoardId(
            // callback
            [callback](bool ok, int boardId) {
                callback(ok ? boardId : std::optional<int>());
            },
            callbackContext
    );
}

std::optional<QSize> CachedDataAccess::getMainWindowSize() {
    const auto [ok, sizeOpt] = localSettingsFile->readMainWindowSize();
    if (!ok)
        return std::nullopt;
    return sizeOpt;
}

void CachedDataAccess::createNewCardWithId(
        const int cardId, const Card &card, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool writeDbOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    routine->addStep([this, routine, cardId, card]() {
        ContinuationContext context(routine);

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
                    ContinuationContext context(routine);
                    routine->writeDbOk = ok;
                    // if ok = false, errorFlag is set in the next step
                },
                this
        );
    }, this);

    // 3. if write DB failed, add to unsaved updates
    routine->addStep([this, routine, cardId, card]() {
        ContinuationContext context(routine);

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
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(!routine->errorFlag);
        finishWriteRequest(requestId);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    class AsyncRoutineWithVars : public AsyncRoutine
    {
    public:
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
    routine->addStep([this, routine, callback, requestId]() {
        callback(routine->dbWriteOk);
        routine->nextStep();
        finishWriteRequest(requestId);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

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
        routine->nextStep();
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        callback(routine->dbWriteOk);
        routine->nextStep();
        finishWriteRequest(requestId);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::createRelationship(
        const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    // 1. update cache
    if (cache.relationships.contains(id)) {
        invokeAction(callbackContext, [callback]() {
            callback(true, false);
        });
        return;
    }

    cache.relationships.insert(id, RelationshipProperties {});

    // 2. write DB
    const int requestId = startWriteRequest();

    queuedDbAccess->createRelationship(
            id,
            // callback
            [=](bool ok, bool created) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createRelationship";
                    const QString updateDetails = printJson(QJsonObject {
                        {"id", id.toString()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
                }

                //
                invokeAction(callbackContext, [callback, ok, created]() {
                    callback(ok, created);
                });

                //
                finishWriteRequest(requestId);
            },
            this
    );
}

void CachedDataAccess::updateBoardsListProperties(
        const BoardsListPropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
        bool fileWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    BoardsListPropertiesUpdate propertiesUpdateForDb = propertiesUpdate;
    propertiesUpdateForDb.boardsOrdering = propertiesUpdate.boardsOrdering;

    routine->addStep([this, routine, propertiesUpdateForDb]() {
        // write DB for `boardsOrdering`
        if (propertiesUpdateForDb.toJson().isEmpty()) {
            ContinuationContext context(routine);
            routine->dbWriteOk = true;
            return;
        }

        queuedDbAccess->updateBoardsListProperties(
                propertiesUpdateForDb,
                // callback
                [=](bool ok) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                        const QString updateTitle = "updateBoardsListProperties";
                        const QString updateDetails = printJson(QJsonObject {
                            {"propertiesUpdate", propertiesUpdateForDb.toJson()}
                        }, false);
                        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
                    }
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, propertiesUpdate]() {
        // write settings file for `lastOpenedBoard`
        ContinuationContext context(routine);

        if (!propertiesUpdate.lastOpenedBoard.has_value()) {
            routine->fileWriteOk = true;
            return;
        }

        bool ok = localSettingsFile->writeLastOpenedBoardId(
                propertiesUpdate.lastOpenedBoard.value());
        if (!ok) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateBoardsListProperties";
            const QString updateDetails = printJson(QJsonObject {
                {"lastOpenedBoard", propertiesUpdate.lastOpenedBoard.value()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
        routine->fileWriteOk = ok;
    }, this);

    routine->addStep([this, routine, callback, requestId]() {
        // final step
        ContinuationContext context(routine);
        callback(routine->dbWriteOk && routine->fileWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::createNewBoardWithId(
        const int boardId, const Board &board,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);
    Q_ASSERT(board.cardIdToNodeRectData.isEmpty()); // new board should have no NodeRect

    const int requestId = startWriteRequest();

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    routine->addStep([this, routine, boardId, board]() {
        ContinuationContext context(routine);

        if (cache.boards.contains(boardId)) {
            qWarning().noquote() << QString("board %1 already exists in cache").arg(boardId);
            context.setErrorFlag();
            return;
        }
        cache.boards.insert(boardId, board);
    }, this);

    // 2. write DB
    routine->addStep([this, routine, boardId, board]() {
        queuedDbAccess->createNewBoardWithId(
                boardId, board,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, boardId, board]() {
        ContinuationContext context(routine);

        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "createNewBoardWithId";
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
                {"boardNodeProperties", board.getNodePropertiesJson()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(!routine->errorFlag && routine->dbWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    // 1. update cache
    if (cache.boards.contains(boardId))
        cache.boards[boardId].updateNodeProperties(propertiesUpdate);

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
        bool fileWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 2. write DB and/or local settings file
    BoardNodePropertiesUpdate propertiesUpdateForDb = propertiesUpdate;
    propertiesUpdateForDb.topLeftPos = std::nullopt;

    routine->addStep([this, routine, boardId, propertiesUpdateForDb]() {
        if (propertiesUpdateForDb.toJson().isEmpty()) {
            ContinuationContext context(routine);
            routine->dbWriteOk = true;
            return;
        }

        queuedDbAccess->updateBoardNodeProperties(
                boardId, propertiesUpdateForDb,
                // callback
                [=](bool ok) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                        const QString updateTitle = "updateBoardNodeProperties";
                        const QString updateDetails = printJson(QJsonObject {
                            {"boardId", boardId},
                            {"propertiesUpdate", propertiesUpdateForDb.toJson()}
                        }, false);
                        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
                    }
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. write settings file for `topLeftPos`
    routine->addStep([this, routine, boardId, propertiesUpdate]() {
        ContinuationContext context(routine);

        if (!propertiesUpdate.topLeftPos.has_value()) {
            routine->fileWriteOk = true;
            return;
        }

        const bool ok = localSettingsFile->writeTopLeftPosOfBoard(
                boardId, propertiesUpdate.topLeftPos.value());
        if (!ok) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateBoardNodeProperties";

            const auto pos = propertiesUpdate.topLeftPos.value();
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
                {"topLeftPos", QJsonArray {pos.x(), pos.y()}}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
        routine->fileWriteOk = ok;
    }, this);


    // 4. final step
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(routine->dbWriteOk && routine->fileWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    routine->start();
}

void CachedDataAccess::removeBoard(
        const int boardId, std::function<void (bool)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    // 1. update cache
    cache.boards.remove(boardId);

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 2. write DB
    routine->addStep([this, routine, boardId]() {
        queuedDbAccess->removeBoard(
                boardId,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, boardId]() {
        ContinuationContext context(routine);

        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "removeBoard";
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(routine->dbWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    // 1. update cache
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.cardIdToNodeRectData.contains(cardId)) {
            board.cardIdToNodeRectData[cardId].update(update);
        }
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        // variables used by the steps of the routine:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 2. write DB
    routine->addStep([this, routine, boardId, cardId, update]() {
        queuedDbAccess->updateNodeRectProperties(
                boardId, cardId, update,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, boardId, cardId, update]() {
        ContinuationContext context(routine);

        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateNodeRectProperties";
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
                {"cardId", cardId},
                {"update", update.toJson()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(routine->dbWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. update cache
    routine->addStep([this, routine, boardId, cardId, nodeRectData]() {
        ContinuationContext context(routine);

        if (cache.boards.contains(boardId)) {
            Board &board = cache.boards[boardId];
            if (board.cardIdToNodeRectData.contains(cardId)) {
                qWarning().noquote()
                        << QString("NodeRect for board %1 & card %2 already exists in cache")
                           .arg(boardId).arg(cardId);
                context.setErrorFlag();
                return;
            }

            board.cardIdToNodeRectData.insert(cardId, nodeRectData);
        }
    }, this);

    // 2. write DB
    routine->addStep([this, routine, boardId, cardId, nodeRectData]() {
        queuedDbAccess->createNodeRect(
                boardId, cardId, nodeRectData,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, boardId, cardId, nodeRectData]() {
        ContinuationContext context(routine);

        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "createNodeRect";
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
                {"cardId", cardId},
                {"nodeRectData", nodeRectData.toJson()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(routine->dbWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    //
    routine->start();
}

void CachedDataAccess::removeNodeRect(
        const int boardId, const int cardId,
        std::function<void (bool)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    const int requestId = startWriteRequest();

    // 1. update cache
    if (cache.boards.contains(boardId))
        cache.boards[boardId].cardIdToNodeRectData.remove(cardId);

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool dbWriteOk;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 2. write DB
    routine->addStep([this, routine, boardId, cardId]() {
        queuedDbAccess->removeNodeRect(
                boardId, cardId,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    routine->dbWriteOk = ok;
                },
                this
        );
    }, this);

    // 3. if step 2 failed, add to unsaved updates
    routine->addStep([this, routine, boardId, cardId]() {
        ContinuationContext context(routine);

        if (!routine->dbWriteOk) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "removeNodeRect";
            const QString updateDetails = printJson(QJsonObject {
                {"boardId", boardId},
                {"cardId", cardId}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
        }
    }, this);

    //
    routine->addStep([this, routine, callback, requestId]() {
        ContinuationContext context(routine);
        callback(routine->dbWriteOk);
        finishWriteRequest(requestId);
    }, callbackContext);

    //
    routine->start();
}

bool CachedDataAccess::saveMainWindowSize(const QSize &size) {
    const bool ok = localSettingsFile->writeMainWindowSize(size);
    if (!ok) {
        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        const QString updateTitle = "saveMainWindowSize";
        const QString updateDetails = printJson(QJsonObject {
            {"size", QJsonArray {size.width(), size.height()}},
        }, false);
        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);
    }
    return ok;
}

int CachedDataAccess::startWriteRequest() {
    const int requestId = ++lastWriteRequestId;
    {
        QWriteLocker locker(&lockForwriteRequestsInProgress);
        writeRequestsInProgress << requestId;
    }
    return requestId;
}

void CachedDataAccess::finishWriteRequest(const int requestId) {
    QWriteLocker locker(&lockForwriteRequestsInProgress);
    writeRequestsInProgress.remove(requestId);
}
