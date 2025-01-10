#include <QDebug>
#include "debounced_db_access.h"
#include "file_access/unsaved_update_records_file.h"
#include "utilities/action_debouncer.h"
#include "utilities/functor.h"
#include "utilities/json_util.h"
#include "utilities/message_box.h"

DebouncedDbAccess::DebouncedDbAccess(AbstractBoardsDataAccess *boardsDataAccess_,
        AbstractCardsDataAccess *cardsDataAccess_,
        std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_, QObject *parent)
            : QObject(parent)
            , boardsDataAccess(boardsDataAccess_)
            , cardsDataAccess(cardsDataAccess_)
            , unsavedUpdateRecordsFile(unsavedUpdateRecordsFile_) {
}

void DebouncedDbAccess::performPendingOperation() {
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

void DebouncedDbAccess::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->queryRelationship(relationshipId, callback, callbackContext);
}

void DebouncedDbAccess::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->queryRelationshipsFromToCards(cardIds, callback, callbackContext);
}

void DebouncedDbAccess::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->getUserLabelsAndRelationshipTypes(callback, callbackContext);
}

void DebouncedDbAccess::requestNewCardId(
        std::function<void (bool, int)> callback, QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->requestNewCardId(callback, callbackContext);
}

void DebouncedDbAccess::queryCustomDataQueries(
        const QSet<int> &dataQueryIds,
        std::function<void (bool, const QHash<int, CustomDataQuery> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->queryCustomDataQueries(dataQueryIds, callback, callbackContext);
}

void DebouncedDbAccess::performCustomCypherQuery(
        const QString &cypher, const QJsonObject &parameters,
        std::function<void (bool, const QVector<QJsonObject> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    cardsDataAccess->performCustomCypherQuery(cypher, parameters, callback, callbackContext);
}

void DebouncedDbAccess::createNewCardWithId(const int cardId, const Card &card) {
    closeDebounceSession();

    cardsDataAccess->createNewCardWithId(
            cardId, card,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createNewCardWithId";
                    const QString updateDetails = printJson(QJsonObject {
                        {"cardId", cardId},
                        {"labels", toJsonArray(card.getLabels())},
                        {"cardProperties", card.getPropertiesJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created card");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateCardProperties(
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
        // 1. prepare write-DB function
        auto functionWriteDb = [this, cardId]() {
            // copy cumulated update data, and clear it
            const auto cumulatedCardPropsUpdate = cumulatedUpdateData.cardPropertiesUpdate;
            cumulatedUpdateData.cardPropertiesUpdate = CardPropertiesUpdate();

            //
            cardsDataAccess->updateCardProperties(
                    cardId,
                    cumulatedCardPropsUpdate,
                    // callback
                    [this, cardId, cumulatedCardPropsUpdate](bool ok) {
                        if (!ok) {
                            const QString time
                                    = QDateTime::currentDateTime().toString(Qt::ISODate);
                            const QString updateTitle = "updateCardProperties";
                            const QString updateDetails = printJson(QJsonObject {
                                {"cardId", cardId},
                                {"propertiesUpdate", cumulatedCardPropsUpdate.toJson()}
                            }, false);
                            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                            showMsgOnDbWriteFailed("card properties update");
                        }
                    },
                    this
            );
        };

        // 2. set update data
        cumulatedUpdateData.cardPropertiesUpdate = cardPropertiesUpdate;

        // 3. create debounce session
        constexpr int separationMsec = 2500;
        currentDebounceSession.emplace(debounceKey, separationMsec, functionWriteDb);
        qInfo().noquote()
                << QString("entered debounce session %1")
                   .arg(currentDebounceSession.value().printKey());

        //
        currentDebounceSession.value().tryAct();
    }
    else { // (already in the debounce session)
        Q_ASSERT(currentDebounceSession.value().key() == debounceKey);

        // 1. cumulate the update data
        cumulatedUpdateData.cardPropertiesUpdate.mergeWith(cardPropertiesUpdate);

        // 2.
        currentDebounceSession.value().tryAct();
    }
}

void DebouncedDbAccess::updateCardLabels(const int cardId, const QSet<QString> &updatedLabels) {
    closeDebounceSession();

    cardsDataAccess->updateCardLabels(
            cardId, updatedLabels,
            // callback:
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateCardLabels";
                    const QString updateDetails = printJson(QJsonObject {
                        {"cardId", cardId},
                        {"updatedLabels", toJsonArray(updatedLabels)}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("updated card labels");
                }
            },
            this
    );
}

void DebouncedDbAccess::createRelationship(const RelationshipId &id) {
    closeDebounceSession();

    cardsDataAccess->createRelationship(
            id,
            // callback:
            [this, id](bool ok, bool /*created*/) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createRelationship";
                    const QString updateDetails = printJson(QJsonObject {
                        {"id", id.toString()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created relationship");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateUserRelationshipTypes(const QStringList &updatedRelTypes) {
    closeDebounceSession();

    cardsDataAccess->updateUserRelationshipTypes(
            updatedRelTypes,
            // callback:
            [this, updatedRelTypes](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateUserRelationshipTypes";
                    const QString updateDetails = printJson(QJsonObject {
                        {"updatedRelTypes", toJsonArray(updatedRelTypes)}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("user-defined list of relationship types");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateUserCardLabels(const QStringList &updatedCardLabels) {
    closeDebounceSession();

    cardsDataAccess->updateUserCardLabels(
            updatedCardLabels,
            // callback
            [this, updatedCardLabels](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateUserCardLabels";
                    const QString updateDetails = printJson(QJsonObject {
                        {"updatedCardLabels", toJsonArray(updatedCardLabels)}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("user-defined list of card labels");
                }
            },
            this
    );
}

void DebouncedDbAccess::createNewCustomDataQueryWithId(
        const int customDataQueryId, const CustomDataQuery &customDataQuery) {
    closeDebounceSession();

    cardsDataAccess->createNewCustomDataQueryWithId(
            customDataQueryId, customDataQuery,
            // callback
            [this, customDataQueryId, customDataQuery](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createNewCustomDataQueryWithId";
                    const QString updateDetails = printJson(QJsonObject {
                        {"customDataQueryId", customDataQueryId},
                        {"customDataQueryProperties", customDataQuery.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created custom data query");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateCustomDataQueryProperties(
        const int customDataQueryId, const CustomDataQueryUpdate &update) {
    const DebounceKey debounceKey {
        DebounceDataCategory::CustomDataQueryProperties,
        QJsonObject {{"customDataQueryId", customDataQueryId}}
    };

    if (currentDebounceSession.has_value()) {
        if (currentDebounceSession.value().key() != debounceKey)
            closeDebounceSession();
    }

    //
    if (!currentDebounceSession.has_value()) {
        // 1. prepare write-DB function
        auto functionWriteDb = [this, customDataQueryId]() {
            // copy cumulated update data, and clear it
            const CustomDataQueryUpdate cumulatedUpdate = cumulatedUpdateData.customDataQueryUpdate;
            cumulatedUpdateData.customDataQueryUpdate = CustomDataQueryUpdate();

            //
            cardsDataAccess->updateCustomDataQueryProperties(
                    customDataQueryId,
                    cumulatedUpdate,
                    // callback
                    [this, customDataQueryId, cumulatedUpdate](bool ok) {
                        if (!ok) {
                            const QString time
                                    = QDateTime::currentDateTime().toString(Qt::ISODate);
                            const QString updateTitle = "updateCustomDataQueryProperties";
                            const QString updateDetails = printJson(QJsonObject {
                                {"customDataQueryId", customDataQueryId},
                                {"propertiesUpdate", cumulatedUpdate.toJson()}
                            }, false);
                            unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                            showMsgOnDbWriteFailed("custom-data-query properties update");
                        }
                    },
                    this
            );
        };

        // 2. set update data
        cumulatedUpdateData.customDataQueryUpdate = update;

        // 3. create debounce session
        constexpr int separationMsec = 2500;
        currentDebounceSession.emplace(debounceKey, separationMsec, functionWriteDb);
        qInfo().noquote()
                << QString("entered debounce session %1")
                   .arg(currentDebounceSession.value().printKey());

        //
        currentDebounceSession.value().tryAct();
    }
    else { // (already in the debounce session)
        Q_ASSERT(currentDebounceSession.value().key() == debounceKey);

        // 1. cumulate the update data
        cumulatedUpdateData.customDataQueryUpdate.mergeWith(update);

        // 2.
        currentDebounceSession.value().tryAct();
    }
}

void DebouncedDbAccess::getWorkspaces(
        std::function<void (bool, const QHash<int, Workspace> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->getWorkspaces(callback, callbackContext);
}

void DebouncedDbAccess::getWorkspacesListProperties(
        std::function<void (bool, WorkspacesListProperties)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->getWorkspacesListProperties(callback, callbackContext);
}

void DebouncedDbAccess::getBoardIdsAndNames(
        std::function<void (bool, const QHash<int, QString> &)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->getBoardIdsAndNames(callback, callbackContext);
}

void DebouncedDbAccess::getBoardsListProperties(
        std::function<void (bool, BoardsListProperties)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->getBoardsListProperties(callback, callbackContext);
}

void DebouncedDbAccess::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->getBoardData(boardId, callback, callbackContext);
}

void DebouncedDbAccess::requestNewBoardId(
        std::function<void (bool, int)> callback, QPointer<QObject> callbackContext) {
    closeDebounceSession();
    boardsDataAccess->requestNewBoardId(callback, callbackContext);
}

void DebouncedDbAccess::createNewWorkspaceWithId(const int workspaceId, const Workspace &workspace) {
    closeDebounceSession();

    boardsDataAccess->createNewWorkspaceWithId(
            workspaceId, workspace,
            // callback
            [this, workspaceId, workspace](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createNewWorkspaceWithId";
                    const QString updateDetails = printJson(QJsonObject {
                        {"workspaceId", workspaceId},
                        {"workspaceNodeProperties", workspace.getNodePropertiesJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created workspace");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateWorkspaceNodeProperties(
        const int workspaceId, const WorkspaceNodePropertiesUpdate &update) {
    closeDebounceSession();

    boardsDataAccess->updateWorkspaceNodeProperties(
            workspaceId, update,
            // callback
            [this, workspaceId, update](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateWorkspaceNodeProperties";
                    const QString updateDetails = printJson(QJsonObject {
                        {"workspaceId", workspaceId},
                        {"update", update.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("workspace properties");
                }
            },
            this
    );
}

void DebouncedDbAccess::removeWorkspace(const int workspaceId) {
    closeDebounceSession();

    boardsDataAccess->removeWorkspace(
            workspaceId,
            // callback
            [this, workspaceId](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "removeWorkspace";
                    const QString updateDetails = printJson(QJsonObject {
                        {"workspaceId", workspaceId}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("removal of workspace");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateBoardsListProperties(
        const BoardsListPropertiesUpdate &propertiesUpdate) {
    closeDebounceSession();

    boardsDataAccess->updateBoardsListProperties(
            propertiesUpdate,
            // callback
            [this, propertiesUpdate](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateBoardsListProperties";
                    const QString updateDetails = printJson(QJsonObject {
                        {"propertiesUpdate", propertiesUpdate.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("boards-list properties");
                }
            },
            this
    );
}

void DebouncedDbAccess::createNewBoardWithId(
        const int boardId, const Board &board, const int workspaceId) {
    closeDebounceSession();

    boardsDataAccess->createNewBoardWithId(
            boardId, board, workspaceId,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createNewBoardWithId";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"boardNodeProperties", board.getNodePropertiesJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created board");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateBoardNodeProperties(
        const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate) {
    closeDebounceSession();

    boardsDataAccess->updateBoardNodeProperties(
            boardId, propertiesUpdate,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateBoardNodeProperties";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"propertiesUpdate", propertiesUpdate.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("board node properties");
                }
            },
            this
    );
}

void DebouncedDbAccess::removeBoard(const int boardId) {
    closeDebounceSession();

    boardsDataAccess->removeBoard(
            boardId,
            // callback
            [this, boardId](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "removeBoard";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("removal of board");
                }
            },
            this

    );
}

void DebouncedDbAccess::updateNodeRectProperties(
        const int boardId, const int cardId, const NodeRectDataUpdate &update) {
    closeDebounceSession();

    boardsDataAccess->updateNodeRectProperties(
            boardId, cardId, update,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateNodeRectProperties";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"cardId", cardId},
                        {"update", update.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("NodeRect properties update");
                }
            },
            this
    );
}

void DebouncedDbAccess::createNodeRect(
        const int boardId, const int cardId, const NodeRectData &nodeRectData) {
    closeDebounceSession();

    boardsDataAccess->createNodeRect(
            boardId, cardId, nodeRectData,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createNodeRect";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"cardId", cardId},
                        {"nodeRectData", nodeRectData.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created NodeRect");
                }
            },
            this
    );
}

void DebouncedDbAccess::removeNodeRect(const int boardId, const int cardId) {
    closeDebounceSession();

    boardsDataAccess->removeNodeRect(
            boardId, cardId,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "removeNodeRect";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"cardId", cardId}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("removal of NodeRect");
                }
            },
            this
    );
}

void DebouncedDbAccess::createDataViewBox(
        const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData) {
    closeDebounceSession();

    boardsDataAccess->createDataViewBox(
            boardId, customDataQueryId, dataViewBoxData,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "createDataViewBox";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"customDataQueryId", customDataQueryId},
                        {"dataViewBoxData", dataViewBoxData.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("created DataViewBox");
                }
            },
            this
    );
}

void DebouncedDbAccess::updateDataViewBoxProperties(
        const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update) {
    closeDebounceSession();

    boardsDataAccess->updateDataViewBoxProperties(
            boardId, customDataQueryId, update,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "updateDataViewBoxProperties";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"customDataQueryId", customDataQueryId},
                        {"update", update.toJson()}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("DataViewBox properties update");
                }
            },
            this
    );
}

void DebouncedDbAccess::removeDataViewBox(const int boardId, const int customDataQueryId) {
    closeDebounceSession();

    boardsDataAccess->removeDataViewBox(
            boardId, customDataQueryId,
            // callback
            [=](bool ok) {
                if (!ok) {
                    const QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
                    const QString updateTitle = "removeDataViewBox";
                    const QString updateDetails = printJson(QJsonObject {
                        {"boardId", boardId},
                        {"customDataQueryId", customDataQueryId}
                    }, false);
                    unsavedUpdateRecordsFile->append(time, updateTitle, updateDetails);

                    showMsgOnDbWriteFailed("removal of DataViewBox");
                }
            },
            this
    );
}

QString DebouncedDbAccess::debounceDataCategoryName(const DebounceDataCategory category) {
    switch (category) {
    case DebounceDataCategory::CardProperties: return "CardProperties";
    case DebounceDataCategory::CustomDataQueryProperties: return "CustomDataQueryProperties";
    }
    return "";
}

void DebouncedDbAccess::closeDebounceSession() {
    std::optional<QString> closedSessionStr;
    if (currentDebounceSession.has_value())
        closedSessionStr = currentDebounceSession.value().printKey();

    currentDebounceSession = std::nullopt;
            // If currentDebounceSession originally has value, this calls destructor of the
            // wrapped DebounceSession.

    if (closedSessionStr.has_value())
        qInfo().noquote() << QString("closed debounce session %1").arg(closedSessionStr.value());
}

void DebouncedDbAccess::showMsgOnDbWriteFailed(const QString &dataName) {
    const auto msg
            = QString("Could not save %1 to DB.\n\nThere is unsaved update. See %2")
              .arg(dataName, unsavedUpdateRecordsFile->getFilePath());
    showWarningMessageBox(nullptr, "Warning", msg);
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
