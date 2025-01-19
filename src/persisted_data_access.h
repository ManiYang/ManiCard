#ifndef PERSISTEDDATAACCESS_H
#define PERSISTEDDATAACCESS_H

#include <QObject>
#include <QPointer>
#include <QReadWriteLock>
#include "app_event_source.h"
#include "models/board.h"
#include "models/card.h"
#include "models/custom_data_query.h"
#include "models/workspace.h"
#include "models/workspaces_list_properties.h"

class DebouncedDbAccess;
class LocalSettingsFile;
class UnsavedUpdateRecordsFile;

//!
//! Accesses persisted data (DB & files), and manages data cache.
//!
//! For read operation, this class
//!   1. gets the parts that are already cached
//!   2. reads from DB or files for the other parts, if any, and if successful, updates the cache.
//! The operation fails only if step 2 is needed and fails.
//!
//! For write operation, this class
//!   1. synchronously updates the cache
//!   2. writes to DB or files, and if failed, adds to the records of unsaved updates.
//!
class PersistedDataAccess : public QObject
{
    Q_OBJECT
public:
    explicit PersistedDataAccess(
            DebouncedDbAccess *debouncedDbAccess_,
            std::shared_ptr<LocalSettingsFile> localSettingsFile_,
            std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile_,
            QObject *parent = nullptr);

    // ==== read ====

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    void queryRelationship(
            const RelId &relationshipId,
            std::function<void (bool ok, const std::optional<RelProperties> &)> callback,
            QPointer<QObject> callbackContext);

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext);

    using StringListPair = std::pair<QStringList, QStringList>;
    void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext);

    void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext);

    void getWorkspaces(
            std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
            QPointer<QObject> callbackContext);

    void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext);

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext);

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext);

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext);

    void queryCustomDataQueries(
            const QSet<int> &customDataQueryIds,
            std::function<void (bool ok, const QHash<int, CustomDataQuery> &dataQueries)> callback,
            QPointer<QObject> callbackContext);

    void performCustomCypherQuery(
            const QString &cypher, const QJsonObject &parameters,
            std::function<void (bool, const QVector<QJsonObject> &)> callback,
            QPointer<QObject> callbackContext);

    std::optional<QSize> getMainWindowSize();

    bool getIsDarkTheme();

    // ==== write ====

    // A write operation fails if data cannot be saved to DB or file. In this case, a record of
    // unsaved update is added and a message box is shown.

    void createNewCardWithId(const int cardId, const Card &card);

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);

    void updateCardLabels(const int cardId, const QSet<QString> &updatedLabels);

    void createNewCustomDataQueryWithId(
            const int customDataQueryId, const CustomDataQuery &customDataQuery);

    void updateCustomDataQueryProperties(
            const int customDataQueryId, const CustomDataQueryUpdate &update);

    //!
    //! The start/end cards must already exist (which is not checked here). Otherwise the cache
    //! will become inconsistent with DB.
    //! It's not an error if the relationship already exists.
    //!
    void createRelationship(const RelationshipId &id);

    void updateUserRelationshipTypes(const QStringList &updatedRelTypes);

    void updateUserCardLabels(const QStringList &updatedCardLabels);

    void createNewWorkspaceWithId(const int workspaceId, const Workspace &workspace);

    void updateWorkspaceNodeProperties(
            const int workspaceId, const WorkspaceNodePropertiesUpdate &update);

    //!
    //! \param workspaceId
    //! \param boardIds: boards of the workspace \e workspaceId
    //!
    void removeWorkspace(const int workspaceId, const QSet<int> &boardIds);

    void updateWorkspacesListProperties(const WorkspacesListPropertiesUpdate &propertiesUpdate);

    //!
    //! \param boardId
    //! \param board
    //! \param workspaceId: If it exists, the relationship (:Workspace)-[:HAS]->(:Board) will be
    //!                     created. It is not an error if \e workspaceId does not exist.
    //!
    void createNewBoardWithId(const int boardId, const Board &board, const int workspaceId);

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate);

    void removeBoard(const int boardId);

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update);

    void createNodeRect(const int boardId, const int cardId, const NodeRectData &nodeRectData);

    void removeNodeRect(const int boardId, const int cardId);

    void createDataViewBox(
            const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData);

    void updateDataViewBoxProperties(
            const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update);

    void removeDataViewBox(const int boardId, const int customDataQueryId);

    void createTopLevelGroupBoxWithId(
            const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData);

    void updateGroupBoxProperties(const int groupBoxId, const GroupBoxNodePropertiesUpdate &update);

    void removeGroupBoxAndReparentChildItems(const int groupBoxId);

    void removeNodeRectFromGroupBox(const int cardId, const int groupBoxId);

    //!
    //! The board having group-box `newParentGroupBox` must have the NodeRect for `cardId`.
    //!
    void addOrReparentNodeRectToGroupBox(const int cardId, const int newParentGroupBox);

    //!
    //! The group-boxes `groupBoxId` & `newParentGroupBoxId` must be on the same board.
    //! `newParentGroupBoxId`

    //!
    //! \param groupBoxId: must exist
    //! \param newParentGroupBoxId:
    //!           + if = -1: `groupBoxId` will be reparented to the board
    //!           + if != -1: must be on the same board as `groupBoxId`, and must not be `groupBoxId`
    //!                       or its descendant
    //!
    void reparentGroupBox(const int groupBoxId, const int newParentGroupBoxId);

    bool saveMainWindowSize(const QSize &size);

    bool saveIsDarkTheme(const bool isDarkTheme);

private:
    DebouncedDbAccess *debouncedDbAccess;
    std::shared_ptr<LocalSettingsFile> localSettingsFile;
    std::shared_ptr<UnsavedUpdateRecordsFile> unsavedUpdateRecordsFile;

    // data cache
    struct Cache
    {
        std::optional<QHash<int, Workspace>> allWorkspaces;
        QHash<int, Board> boards;
        QHash<int, Card> cards;
        QHash<RelationshipId, RelationshipProperties> relationships;
        QHash<int, CustomDataQuery> customDataQueries;

        std::optional<QStringList> userLabelsList;
        std::optional<QStringList> userRelTypesList;

        std::optional<bool> isDarkTheme;
    };
    Cache cache;

    //
    void showMsgOnFailedToSaveToFile(const QString &dataName);
};

#endif // PERSISTEDDATAACCESS_H
