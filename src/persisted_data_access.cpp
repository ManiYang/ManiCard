#include <QApplication>
#include <QDateTime>
#include <QReadLocker>
#include <QStandardPaths>
#include <QWriteLocker>
#include "persisted_data_access.h"
#include "db_access/debounced_db_access.h"
#include "file_access/local_settings_file.h"
#include "file_access/unsaved_update_records_file.h"
#include "utilities/async_routine.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

PersistedDataAccess::PersistedDataAccess(
        DebouncedDbAccess *debouncedDbAccess_,
        std::shared_ptr<LocalSettingsFile> localSettingsFile_,
        std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
        QObject *parent)
            : QObject(parent)
            , debouncedDbAccess(debouncedDbAccess_)
            , localSettingsFile(localSettingsFile_)
            , unsavedUpdateRecordsFile(unsavedUpdateRecordsFile_) {
}

void PersistedDataAccess::clearCache() {
    cache.clear();
}

void PersistedDataAccess::queryCards(
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
    routine->setName("PersistedDataAccess::queryCards");

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
        if (cardsToQuery.isEmpty()) {
            routine->dbQueryOk = true;
            routine->nextStep();
            return;
        }

        debouncedDbAccess->queryCards(
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

void PersistedDataAccess::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    // 1. get the parts that are already cached
    if (cache.relationships.contains(relationshipId)) {
        const auto result = cache.relationships.value(relationshipId);
        invokeAction(callbackContext, [callback, result]() {
            callback(true, result);
        });
        return;
    }

    // 2. query DB
    debouncedDbAccess->queryRelationship(
            relationshipId,
            // callback:
            [=](bool ok, const std::optional<RelProperties> &propertiesOpt) {
                // update cache
                if (ok && propertiesOpt.has_value())
                    cache.relationships.insert(relationshipId, propertiesOpt.value());

                //
                invokeAction(callbackContext, [callback, ok, propertiesOpt]() {
                    callback(ok, propertiesOpt);
                });
            },
            this
    );
}

void PersistedDataAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    debouncedDbAccess->queryRelationshipsFromToCards(
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

void PersistedDataAccess::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    if (cache.userLabelsList.has_value() && cache.userRelTypesList.has_value()) {
        invokeAction(callbackContext, [this, callback]() {
            callback(true, {cache.userLabelsList.value(), cache.userRelTypesList.value()});
        });
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList labels;
        QStringList relTypes;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine]() {
        // read DB
        debouncedDbAccess->getUserLabelsAndRelationshipTypes(
                //callback
                [this, routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                    }
                    else {
                        routine->labels = labelsAndRelTypes.first;
                        routine->relTypes = labelsAndRelTypes.second;

                        // update cache
                        cache.userLabelsList = routine->labels;
                        cache.userRelTypesList = routine->relTypes;
                    }
                },
                this
        );

    }, this);

    routine->addStep([routine, callback]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag)
            callback(false, {QStringList(), QStringList()});
        else
            callback(true, {routine->labels, routine->relTypes});
    }, callbackContext);

    routine->start();
}

void PersistedDataAccess::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    debouncedDbAccess->requestNewCardId(
            // callback
            [callback](bool ok, int cardId) {
                callback(ok ? cardId : std::optional<int>());
            },
            callbackContext
    );
}

void PersistedDataAccess::getWorkspaces(
        std::function<void (bool, const QHash<int, Workspace> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    // 1. get the parts that are already cached
    if (cache.allWorkspaces.has_value()) {
        invokeAction(callbackContext, [callback, this]() {
            callback(true, cache.allWorkspaces.value());
        });
        return;
    }

    // 2. read DB & files
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QHash<int, Workspace> workspacesData;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine]() {
        // query DB
        debouncedDbAccess->getWorkspaces(
                // callback
                [routine](bool ok, const QHash<int, Workspace> &workspaces) {
                    ContinuationContext context(routine);
                    if (ok)
                        routine->workspacesData = workspaces;
                    else
                        context.setErrorFlag();
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // read local settings file
        ContinuationContext context(routine);

        const auto workspaceIds = keySet(routine->workspacesData);
        for (const int workspaceId: workspaceIds) {
            const auto [ok, boardIdOpt] = localSettingsFile->readLastOpenedBoardIdOfWorkspace(workspaceId);
            if (!ok) {
                context.setErrorFlag();
                break;
            }
            if (boardIdOpt.has_value())
                routine->workspacesData[workspaceId].lastOpenedBoardId = boardIdOpt.value();
        }
    }, this);

    routine->addStep([this, routine, callback, callbackContext]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag) {
            callback(false, {});
        }
        else {
            // update cache
            cache.allWorkspaces = routine->workspacesData;

            //
            const QHash<int, Workspace> workspacesData = routine->workspacesData;
            invokeAction(callbackContext, [callback, workspacesData]() {
                callback(true, workspacesData);
            });
        }
    }, this);

    routine->start();
}

void PersistedDataAccess::getWorkspacesListProperties(
        std::function<void (bool, WorkspacesListProperties)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        WorkspacesListProperties properties;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get workspaces ordering from DB
        debouncedDbAccess->getWorkspacesListProperties(
                // callback
                [routine](bool ok, WorkspacesListProperties properties) {
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
        // get last-opened workspace from local settings file
        ContinuationContext context(routine);

        const auto [ok, workspaceIdOpt] = localSettingsFile->readLastOpenedWorkspaceId();
        if (!ok) {
            context.setErrorFlag();
        }
        else {
            if (workspaceIdOpt.has_value())
                routine->properties.lastOpenedWorkspace = workspaceIdOpt.value();
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

void PersistedDataAccess::getBoardIdsAndNames(
        std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    debouncedDbAccess->getBoardIdsAndNames(
            // callback
            [callback](bool ok, const QHash<int, QString> &idToName) {
                callback(ok, idToName);
            },
            callbackContext
    );
}

void PersistedDataAccess::getBoardData(
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
    routine->setName("PersistedDataAccess::getBoardData");

    // 2. query DB
    routine->addStep([this, routine, boardId]() {
        debouncedDbAccess->getBoardData(
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

void PersistedDataAccess::requestNewBoardId(
        std::function<void (std::optional<int>)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    debouncedDbAccess->requestNewBoardId(
            // callback
            [callback](bool ok, int boardId) {
                callback(ok ? boardId : std::optional<int>());
            },
            callbackContext
    );
}

void PersistedDataAccess::queryCustomDataQueries(
        const QSet<int> &customDataQueryIds,
        std::function<void (bool, const QHash<int, CustomDataQuery> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QHash<int, CustomDataQuery> result;
    };
    auto *routine = new AsyncRoutineWithVars;

    // 1. get the parts that are already cached
    for (const int id: customDataQueryIds) {
        if (cache.customDataQueries.contains(id))
            routine->result.insert(id, cache.customDataQueries.value(id));
    }

    // 2. query DB for the other parts
    //   + if successful: update cache
    //   + if failed: whole process fails
    const QSet<int> idsToQuery = customDataQueryIds - keySet(routine->result);

    routine->addStep([this, idsToQuery, routine]() {
        if (idsToQuery.isEmpty()) {
            routine->nextStep();
            return;
        }

        debouncedDbAccess->queryCustomDataQueries(
                idsToQuery,
                // callback:
                [this, routine](bool queryOk, const QHash<int, CustomDataQuery> &dataQueriesFromDb) {
                    ContinuationContext context(routine);

                    if (queryOk) {
                        mergeWith(routine->result, dataQueriesFromDb);
                        mergeWith(cache.customDataQueries, dataQueriesFromDb); // update cache
                    }
                    else {
                        context.setErrorFlag();
                    }
                },
                this
        );
    }, this);

    routine->addStep([routine, callback]() {
        ContinuationContext context(routine);

        if (!routine->errorFlag)
            callback(true, routine->result);
        else
            callback(false, {});
    }, callbackContext);

    routine->start();
}

void PersistedDataAccess::performCustomCypherQuery(
        const QString &cypher, const QJsonObject &parameters,
        std::function<void (bool, const QVector<QJsonObject> &)> callback,
        QPointer<QObject> callbackContext) {
    Q_ASSERT(callback);

    debouncedDbAccess->performCustomCypherQuery(cypher, parameters, callback, callbackContext);
}

std::optional<QRect> PersistedDataAccess::getMainWindowSizePos() {
    const auto [ok, rectOpt] = localSettingsFile->readMainWindowSizePos();
    if (!ok)
        return std::nullopt;
    return rectOpt;
}

bool PersistedDataAccess::getIsDarkTheme() {
    // 1. gets the parts that are already cached
    if (cache.isDarkTheme.has_value())
        return cache.isDarkTheme.value();

    // 2. reads from file and update the cache.
    const auto [ok, isDarkThemeOpt] = localSettingsFile->readIsDarkTheme();

    bool isDarkTheme = ok ? isDarkThemeOpt.value_or(false) : false;
    cache.isDarkTheme = isDarkTheme;
    return isDarkTheme;
}

bool PersistedDataAccess::getAutoAdjustCardColorsForDarkTheme() {
    // 1. gets the parts that are already cached
    if (cache.autoAdjustCardColorsForDarkTheme.has_value())
        return cache.autoAdjustCardColorsForDarkTheme.value();

    // 2. reads from file and update the cache.
    const auto [ok, autoAdjustOpt] = localSettingsFile->readAutoAdjustCardColorForDarkTheme();

    bool autoAdjust = ok ? autoAdjustOpt.value_or(false) : false;
    cache.autoAdjustCardColorsForDarkTheme = autoAdjust;
    return autoAdjust;
}

QString PersistedDataAccess::getExportOutputDir() {
    // 1. gets the parts that are already cached

    // 2. reads from file and update the cache.
    const auto [ok, outputDirOpt] = localSettingsFile->readExportOutputDirectory();

    QStringList fallbackPaths = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
    if (fallbackPaths.isEmpty())
        fallbackPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (fallbackPaths.isEmpty()) {
        Q_ASSERT(false);
        return qApp->applicationDirPath();
    }

    if (!ok || !outputDirOpt.has_value())
        return fallbackPaths.at(0);
    else
        return outputDirOpt.value();
}

void PersistedDataAccess::createNewCardWithId(const int cardId, const Card &card) {
    // 1. update cache synchronously
    if (cache.cards.contains(cardId)) {
        qWarning().noquote()
                << QString("card with ID %1 already exists in cache").arg(cardId);
        return;
    }
    cache.cards.insert(cardId, card);

    // 2. write DB
    debouncedDbAccess->createNewCardWithId(cardId, card);
}

void PersistedDataAccess::updateCardProperties(
        const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate) {
    // 1. update cache synchronously
    if (cache.cards.contains(cardId))
        cache.cards[cardId].updateProperties(cardPropertiesUpdate);

    // 2. write DB
    debouncedDbAccess->updateCardProperties(cardId, cardPropertiesUpdate);
}

void PersistedDataAccess::updateCardLabels(
        const int cardId, const QSet<QString> &updatedLabels) {
    // 1. update cache synchronously
    if (cache.cards.contains(cardId))
        cache.cards[cardId].setLabels(updatedLabels);

    // 2. write DB
    debouncedDbAccess->updateCardLabels(cardId, updatedLabels);
}

void PersistedDataAccess::createNewCustomDataQueryWithId(
        const int customDataQueryId, const CustomDataQuery &customDataQuery) {
    // 1. update cache synchronously
    if (cache.customDataQueries.contains(customDataQueryId)) {
        qWarning().noquote()
                << QString("custom-data-query with ID %1 already exists in cache")
                   .arg(customDataQueryId);
        return;
    }
    cache.customDataQueries.insert(customDataQueryId, customDataQuery);

    // 2. write DB
    debouncedDbAccess->createNewCustomDataQueryWithId(customDataQueryId, customDataQuery);
}

void PersistedDataAccess::updateCustomDataQueryProperties(
        const int customDataQueryId, const CustomDataQueryUpdate &update) {
    // 1. update cache synchronously
    if (cache.customDataQueries.contains(customDataQueryId))
        cache.customDataQueries[customDataQueryId].update(update);

    // 2. write DB
    debouncedDbAccess->updateCustomDataQueryProperties(customDataQueryId, update);
}

void PersistedDataAccess::createRelationship(const RelationshipId &id) {
    if (cache.relationships.contains(id))
        return;

    // 1. update cache synchronously
    cache.relationships.insert(id, RelationshipProperties {});

    // 2. write DB
    debouncedDbAccess->createRelationship(id);
}

void PersistedDataAccess::updateUserRelationshipTypes(const QStringList &updatedRelTypes) {
    // 1. update cache synchronously
    cache.userRelTypesList = updatedRelTypes;

    // 2. write DB
    debouncedDbAccess->updateUserRelationshipTypes(updatedRelTypes);
}

void PersistedDataAccess::updateUserCardLabels(const QStringList &updatedCardLabels) {
    // 1. update cache synchronously
    cache.userLabelsList = updatedCardLabels;

    // 2. write DB
    debouncedDbAccess->updateUserCardLabels(updatedCardLabels);
}

void PersistedDataAccess::createNewWorkspaceWithId(const int workspaceId, const Workspace &workspace) {
    Q_ASSERT(workspace.boardIds.isEmpty()); // new workspace should have no board

    // 1. update cache synchronously
    if (cache.allWorkspaces.has_value())
        cache.allWorkspaces.value().insert(workspaceId, workspace);

    // 2. write DB
    debouncedDbAccess->createNewWorkspaceWithId(workspaceId, workspace);
}

void PersistedDataAccess::updateWorkspaceNodeProperties(
        const int workspaceId, const WorkspaceNodePropertiesUpdate &update) {
    // 1. update cache synchronously
    if (cache.allWorkspaces.has_value()) {
        cache.allWorkspaces.value()[workspaceId].updateNodeProperties(update);
    }

    // 2. write DB (for properties other than `lastOpenedBoardId`)
    WorkspaceNodePropertiesUpdate updateForDb = update;
    updateForDb.lastOpenedBoardId = std::nullopt;

    if (!updateForDb.toJson().isEmpty())
        debouncedDbAccess->updateWorkspaceNodeProperties(workspaceId, updateForDb);

    // 3. write settings file (for property `lastOpenedBoardId`)
    if (update.lastOpenedBoardId.has_value()) {
        const bool ok = localSettingsFile->writeLastOpenedBoardIdOfWorkspace(
                workspaceId, update.lastOpenedBoardId.value());
        if (!ok) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateWorkspaceNodeProperties";

            const QString updateDetails = printJson(QJsonObject {
                {"workspaceId", workspaceId},
                {"lastOpenedBoardId", update.lastOpenedBoardId.value()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

            showMsgOnFailedToSaveToFile("last-opened board of workspace");
        }
    }
}

void PersistedDataAccess::removeWorkspace(const int workspaceId, const QSet<int> &boardIds) {
    // 1. update cache synchronously
    if (cache.allWorkspaces.has_value()) {
        QHash<int, Workspace> &allWorkspaces = cache.allWorkspaces.value();
        if (!allWorkspaces.contains(workspaceId)) {
            qWarning().noquote() << QString("workspace %1 does not exist in cache").arg(workspaceId);
        }
        else {
            if (allWorkspaces[workspaceId].boardIds != boardIds) {
                qWarning().noquote()
                        << QString("workspace %1 in cache contains a different set of boards")
                           .arg(workspaceId);
            }
            allWorkspaces.remove(workspaceId);
        }
    }

    for (const int boardId: boardIds)
        cache.boards.remove(boardId);

    // 2. write DB
    debouncedDbAccess->removeWorkspace(workspaceId);
}

void PersistedDataAccess::updateWorkspacesListProperties(
        const WorkspacesListPropertiesUpdate &propertiesUpdate) {
    // write DB for property `workspacesOrdering`
    WorkspacesListPropertiesUpdate propertiesUpdateForDb;
    propertiesUpdateForDb.workspacesOrdering = propertiesUpdate.workspacesOrdering;

    if (!propertiesUpdateForDb.toJson().isEmpty())
        debouncedDbAccess->updateWorkspacesListProperties(propertiesUpdateForDb);

    // write settings file for `lastOpenedWorkspace`
    if (propertiesUpdate.lastOpenedWorkspace.has_value()) {
        const bool ok = localSettingsFile->writeLastOpenedWorkspaceId(
                propertiesUpdate.lastOpenedWorkspace.value());

        if (!ok) {
            const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
            const QString updateTitle = "updateWorkspacesListProperties";
            const QString updateDetails = printJson(QJsonObject {
                {"lastOpenedWorkspace", propertiesUpdate.lastOpenedWorkspace.value()}
            }, false);
            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

            showMsgOnFailedToSaveToFile("last-opened workspace");
        }
    }
}

void PersistedDataAccess::createNewBoardWithId(
        const int boardId, const Board &board, const int workspaceId) {
    Q_ASSERT(board.cardIdToNodeRectData.isEmpty()); // new board should have no NodeRect

    if (cache.boards.contains(boardId)) {
        qWarning().noquote() << QString("board %1 already exists in cache").arg(boardId);
        return;
    }

    // 1. update cache synchronously
    cache.boards.insert(boardId, board);

    if (cache.allWorkspaces.has_value()) {
        if (cache.allWorkspaces.value().contains(workspaceId))
            cache.allWorkspaces.value()[workspaceId].boardIds << boardId;
    }

    // 2. write DB
    debouncedDbAccess->createNewBoardWithId(boardId, board, workspaceId);
}

void PersistedDataAccess::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId))
        cache.boards[boardId].updateNodeProperties(propertiesUpdate);

    // 2. write DB and/or local settings file
    BoardNodePropertiesUpdate propertiesUpdateForDb = propertiesUpdate;
    propertiesUpdateForDb.topLeftPos = std::nullopt;

    if (!propertiesUpdateForDb.toJson().isEmpty())
        debouncedDbAccess->updateBoardNodeProperties(boardId, propertiesUpdateForDb);

    // 3. write settings file for `topLeftPos`
    if (propertiesUpdate.topLeftPos.has_value()) {
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

            showMsgOnFailedToSaveToFile("top-left coordinates of board");
        }
    }
}

void PersistedDataAccess::removeBoard(const int boardId) {
    // 1. update cache synchronously
    cache.boards.remove(boardId);

    if (cache.allWorkspaces.has_value()) {
        QHash<int, Workspace> &allWorkspaces = cache.allWorkspaces.value();
        for (auto it = allWorkspaces.begin(); it != allWorkspaces.end(); ++it) {
            Workspace &workspace = it.value();
            const bool found = workspace.boardIds.remove(boardId);
            if (found)
                break;
        }
    }

    // 2. write DB & files
    debouncedDbAccess->removeBoard(boardId);
    localSettingsFile->removeBoard(boardId);
}

void PersistedDataAccess::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.cardIdToNodeRectData.contains(cardId)) {
            board.cardIdToNodeRectData[cardId].update(update);
        }
    }

    // 2. write DB
    debouncedDbAccess->updateNodeRectProperties(boardId, cardId, update);
}

void PersistedDataAccess::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.cardIdToNodeRectData.contains(cardId)) {
            qWarning().noquote()
                    << QString("NodeRect for board %1 & card %2 already exists in cache")
                       .arg(boardId).arg(cardId);
            return;
        }

        board.cardIdToNodeRectData.insert(cardId, nodeRectData);
    }

    // 2. write DB
    debouncedDbAccess->createNodeRect(boardId, cardId, nodeRectData);
}

void PersistedDataAccess::removeNodeRect(const int boardId, const int cardId) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId))
        cache.boards[boardId].cardIdToNodeRectData.remove(cardId);

    // 2. write DB
    debouncedDbAccess->removeNodeRect(boardId, cardId);
}

void PersistedDataAccess::createDataViewBox(
        const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.customDataQueryIdToDataViewBoxData.contains(customDataQueryId)) {
            qWarning().noquote()
                    << QString("DataViewBox for board %1 & custom-data-query %2 "
                               "already exists in cache")
                       .arg(boardId).arg(customDataQueryId);
            return;
        }

        board.customDataQueryIdToDataViewBoxData.insert(customDataQueryId, dataViewBoxData);
    }

    // 2. write DB
    debouncedDbAccess->createDataViewBox(boardId, customDataQueryId, dataViewBoxData);
}

void PersistedDataAccess::updateDataViewBoxProperties(
        const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.customDataQueryIdToDataViewBoxData.contains(customDataQueryId)) {
            board.customDataQueryIdToDataViewBoxData[customDataQueryId].update(update);
        }
    }

    // 2. write DB
    debouncedDbAccess->updateDataViewBoxProperties(boardId, customDataQueryId, update);
}

void PersistedDataAccess::removeDataViewBox(const int boardId, const int customDataQueryId) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId))
        cache.boards[boardId].customDataQueryIdToDataViewBoxData.remove(customDataQueryId);

    // 2. write DB
    debouncedDbAccess->removeDataViewBox(boardId, customDataQueryId);
}

void PersistedDataAccess::createTopLevelGroupBoxWithId(
        const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData) {
    Q_ASSERT(groupBoxId != -1);

    // 1. update cache synchronously
    for (auto it = cache.boards.constBegin(); it != cache.boards.constEnd(); ++it) {
        if (it.value().groupBoxIdToData.contains(groupBoxId)) {
            qWarning().noquote() << QString("GroupBox %1 already exists").arg(groupBoxId);
            return;
        }
    }

    if (cache.boards.contains(boardId))
        cache.boards[boardId].groupBoxIdToData.insert(groupBoxId, groupBoxData);

    // 2. write DB
    debouncedDbAccess->createTopLevelGroupBoxWithId(boardId, groupBoxId, groupBoxData);
}

void PersistedDataAccess::updateGroupBoxProperties(
        const int groupBoxId, const GroupBoxNodePropertiesUpdate &update) {
    Q_ASSERT(groupBoxId != -1);

    // 1. update cache synchronously
    for (auto it = cache.boards.begin(); it != cache.boards.end(); ++it) {
        Board &board = it.value();
        if (board.groupBoxIdToData.contains(groupBoxId)) {
            board.groupBoxIdToData[groupBoxId].updateNodeProperties(update);
            break;
        }
    }

    // 2. write DB
    debouncedDbAccess->updateGroupBoxProperties(groupBoxId, update);
}

void PersistedDataAccess::removeGroupBoxAndReparentChildItems(const int groupBoxId) {
    Q_ASSERT(groupBoxId != -1);

    // 1. update cache synchronously
    for (auto it = cache.boards.begin(); it != cache.boards.end(); ++it) {
        Board &board = it.value();
        if (board.groupBoxIdToData.contains(groupBoxId)) {
            const int parentGroupBoxId = board.findParentGroupBoxOfGroupBox(groupBoxId); // can be -1
            if (parentGroupBoxId != -1) { // parent is a group-box
                const auto childGroupBoxes = board.groupBoxIdToData[groupBoxId].childGroupBoxes;
                const auto childCards = board.groupBoxIdToData[groupBoxId].childCards;

                board.groupBoxIdToData[parentGroupBoxId].childGroupBoxes += childGroupBoxes;
                board.groupBoxIdToData[parentGroupBoxId].childCards += childCards;
            }
            board.groupBoxIdToData.remove(groupBoxId);
            break;
        }
    }

    // 2. write DB
    debouncedDbAccess->removeGroupBoxAndReparentChildItems(groupBoxId);
}

void PersistedDataAccess::removeNodeRectFromGroupBox(const int cardId, const int groupBoxId) {
    Q_ASSERT(groupBoxId != -1);

    // 1. update cache synchronously
    for (auto it = cache.boards.begin(); it != cache.boards.end(); ++it) {
        Board &board = it.value();
        if (board.groupBoxIdToData.contains(groupBoxId)) {
            board.groupBoxIdToData[groupBoxId].childCards.remove(cardId);
            break;
        }
    }

    // 2. write DB
    debouncedDbAccess->removeNodeRectFromGroupBox(cardId);
}

void PersistedDataAccess::addOrReparentNodeRectToGroupBox(
        const int cardId, const int newParentGroupBox) {
    Q_ASSERT(cardId != -1);
    Q_ASSERT(newParentGroupBox != -1);

    // 1. update cache synchronously
    // -- find board containing `newParentGroupBox`
    int boardIdFoundInCache = -1;
    for (auto it = cache.boards.constBegin(); it != cache.boards.constEnd(); ++it) {
        if (it.value().groupBoxIdToData.contains(newParentGroupBox)) {
            boardIdFoundInCache = it.key();
            break;
        }
    }

    if (boardIdFoundInCache != -1) {
        Board &board = cache.boards[boardIdFoundInCache];
        if (!board.cardIdToNodeRectData.contains(cardId)) {
            qWarning().noquote()
                    << QString("in cache, board %1 does not have NodeRect for card %1")
                       .arg(boardIdFoundInCache).arg(cardId);
            return;
        }

        // remove from original parent group-box, if found
        const int originalParentGroupBox = board.findParentGroupBoxOfCard(cardId); // can be -1
        if (originalParentGroupBox != -1)
            board.groupBoxIdToData[originalParentGroupBox].childCards.remove(cardId);

        // add to `newParentGroupBox`
        board.groupBoxIdToData[newParentGroupBox].childCards << cardId;
    }

    // 2. write DB
    debouncedDbAccess->addOrReparentNodeRectToGroupBox(cardId, newParentGroupBox);
}

void PersistedDataAccess::reparentGroupBox(const int groupBoxId, const int newParentGroupBoxId) {
    Q_ASSERT(groupBoxId != -1);

    // 1. update cache synchronously
    // -- find board containing `groupBoxId`
    int boardIdFoundInCache = -1;
    for (auto it = cache.boards.constBegin(); it != cache.boards.constEnd(); ++it) {
        if (it.value().groupBoxIdToData.contains(groupBoxId)) {
            boardIdFoundInCache = it.key();
            break;
        }
    }

    if (boardIdFoundInCache != -1) {
        Board &board = cache.boards[boardIdFoundInCache];

        // checks
        const int originalParent = board.findParentGroupBoxOfGroupBox(groupBoxId); // can be -1

        if (newParentGroupBoxId != -1) {
            if (!board.groupBoxIdToData.contains(newParentGroupBoxId)) {
                qWarning().noquote()
                        << QString("group-boxes %1 & %2 are not on the same board")
                           .arg(groupBoxId).arg(newParentGroupBoxId);
                return;
            }

            if (originalParent == newParentGroupBoxId)
                return; // `newParentGroupBoxId` is already the parent of groupBoxId

            if (newParentGroupBoxId == groupBoxId) {
                qWarning().noquote()
                        << QString("cannot reparent group-box %1 to itself").arg(groupBoxId);
                return;
            }

            if (board.isGroupBoxADescendantOfGroupBox(newParentGroupBoxId, groupBoxId)) {
                qWarning().noquote()
                        << QString("cannot reparent group-box %1 to one of its descendants")
                           .arg(groupBoxId);
                return;
            }
        }
        else {
            if (originalParent == -1)
                return; // `groupBoxId` is already a child of the board
        }

        //
        if (originalParent != -1)
            board.groupBoxIdToData[originalParent].childGroupBoxes.remove(groupBoxId);

        if (newParentGroupBoxId != -1) {
            Q_ASSERT(board.groupBoxIdToData.contains(newParentGroupBoxId));
            board.groupBoxIdToData[newParentGroupBoxId].childGroupBoxes << groupBoxId;
        }
    }

    // 2. write DB
    debouncedDbAccess->reparentGroupBox(groupBoxId, newParentGroupBoxId);
}

void PersistedDataAccess::createSettingBox(const int boardId, const SettingBoxData &settingBoxData) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        if (board.hasSettingBoxFor(settingBoxData.targetType, settingBoxData.category)) {
            qWarning().noquote()
                    << QString("setting-box for (%1, %2) & Board %3 already exists in cache")
                       .arg(settingBoxData.getTargetTypeId(), settingBoxData.getCategoryId())
                       .arg(boardId);
            return;
        }

        board.settingBoxesData << settingBoxData;
    }

    // 2. write DB
    debouncedDbAccess->createSettingBox(boardId, settingBoxData);
}

void PersistedDataAccess::updateSettingBoxProperties(
        const int boardId, const SettingTargetType targetType,
        const SettingCategory category, const SettingBoxDataUpdate &update) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        board.updateSettingBoxData(targetType, category, update);
    }

    // 2. write DB
    debouncedDbAccess->updateSettingBoxProperties(boardId, targetType, category, update);
}

void PersistedDataAccess::removeSettingBox(
        const int boardId, const SettingTargetType targetType, const SettingCategory category) {
    // 1. update cache synchronously
    if (cache.boards.contains(boardId)) {
        Board &board = cache.boards[boardId];
        board.removeSettingBoxData(targetType, category);
    }

    // 2. write DB
    debouncedDbAccess->removeSettingBox(boardId, targetType, category);
}

void PersistedDataAccess::saveMainWindowSizePos(const QRect &rect) {
    const bool ok = localSettingsFile->writeMainWindowSizePos(rect);
    if (!ok) {
        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        const QString updateTitle = "saveMainWindowSizePos";
        const QString updateDetails = printJson(QJsonObject {
            {"rectTopLeft", QJsonArray {rect.x(), rect.y()}},
            {"rectSize", QJsonArray {rect.width(), rect.height()}},
        }, false);
        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

        showMsgOnFailedToSaveToFile("main-window size and position");
    }
}

void PersistedDataAccess::saveIsDarkTheme(const bool isDarkTheme) {
    // synchronously updates the cache
    cache.isDarkTheme = isDarkTheme;

    // write file
    const bool ok = localSettingsFile->writeIsDarkTheme(isDarkTheme);
    if (!ok) {
        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        const QString updateTitle = "saveIsDarkTheme";
        const QString updateDetails = printJson(QJsonObject {
            {"isDarkTheme", isDarkTheme},
        }, false);
        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

        showMsgOnFailedToSaveToFile("theme option");
    }
}

void PersistedDataAccess::saveAutoAdjustCardColorsForDarkTheme(const bool autoAdjust) {
    // synchronously updates the cache
    cache.autoAdjustCardColorsForDarkTheme = autoAdjust;

    // write file
    const bool ok = localSettingsFile->writeAutoAdjustCardColorForDarkTheme(autoAdjust);
    if (!ok) {
        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        const QString updateTitle = "saveAutoAdjustCardColorsForDarkTheme";
        const QString updateDetails = printJson(QJsonObject {
            {"autoAdjust", autoAdjust},
        }, false);
        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

        showMsgOnFailedToSaveToFile("appearance option");
    }
}

void PersistedDataAccess::saveExportOutputDir(const QString &outputDir) {
    // synchronously updates the cache

    // write file
    const bool ok = localSettingsFile->writeExportOutputDirectory(outputDir);
    if (!ok) {
        const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
        const QString updateTitle = "saveExportOutputDir";
        const QString updateDetails = printJson(QJsonObject {
            {"outputDir", outputDir},
        }, false);
        unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

        showMsgOnFailedToSaveToFile("export option");
    }
}

void PersistedDataAccess::showMsgOnFailedToSaveToFile(const QString &dataName) {
    const auto msg
            = QString("Could not save %1 to file.\n\nThere is unsaved update. See %2")
              .arg(dataName, unsavedUpdateRecordsFile->getFilePath());
    showWarningMessageBox(nullptr, "Warning", msg);
}
