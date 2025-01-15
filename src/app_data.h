#ifndef APPDATA_H
#define APPDATA_H

#include <QObject>
#include <QWidget>
#include "app_data_readonly.h"
#include "app_event_source.h"
#include "models/board.h"
#include "models/card.h"
#include "models/relationship.h"

class PersistedDataAccess;

//!
//! Data managed here are categorized into
//!  - independent data
//!    - persisted
//!    - non-persisted
//!  - derived data (computed from independent data)
//!
//! Each method for updating independent data does the following synchronously:
//!  1. calls method of \c PersistedDataAccess for persisted independent data
//!  2. computes all derived variables and emit "updated" signals for updated ones
//!
//! "Get" methods can be asynchronous.
//!
class AppData : public AppDataReadonly
{
    Q_OBJECT
public:
    explicit AppData(PersistedDataAccess *persistedDataAccess_, QObject *parent = nullptr);

    // ==== persisted data ====

    // ---- persisted data: get ----

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationship(
            const RelId &relationshipId,
            std::function<void (bool ok, const std::optional<RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

    void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) override;

    using StringListPair = std::pair<QStringList, QStringList>;
    void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext) override;

    void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext) override;

    void getWorkspaces(
           std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
           QPointer<QObject> callbackContext) override;

    void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) override;

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext) override;

    std::optional<QSize> getMainWindowSize() override;

    void queryCustomDataQueries(
                const QSet<int> &customDataQueryIds,
                std::function<void (bool ok, const QHash<int, CustomDataQuery> &dataQueries)> callback,
                QPointer<QObject> callbackContext) override;

    void performCustomCypherQuery(
            const QString &cypher, const QJsonObject &parameters,
            std::function<void (bool ok, const QVector<QJsonObject> &result)> callback,
            QPointer<QObject> callbackContext) override;

    // ---- persisted data: update ----

    // If persistence fails, a record of unsaved update is added and a message box is shown.

    void createNewCardWithId(const EventSource &eventSrc, const int cardId, const Card &card);

    void updateCardProperties(
            const EventSource &eventSrc,
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);

    void updateCardLabels(
            const EventSource &eventSrc, const int cardId, const QSet<QString> &updatedLabels);

    void createNewCustomDataQueryWithId(
            const EventSource &eventSrc,
            const int customDataQueryId, const CustomDataQuery &customDataQuery);

    void updateCustomDataQueryProperties(
            const EventSource &eventSrc,
            const int customDataQueryId, const CustomDataQueryUpdate &update);

    //!
    //! The start/end cards must already exist (which is not checked here).
    //! It's not an error if the relationship already exists.
    //!
    void createRelationship(const EventSource &eventSrc, const RelationshipId &id);

    void updateUserRelationshipTypes(
            const EventSource &eventSrc, const QStringList &updatedRelTypes);

    void updateUserCardLabels(
            const EventSource &eventSrc, const QStringList &updatedCardLabels);

    void createNewWorkspaceWithId(
            const EventSource &eventSrc, const int workspaceId, const Workspace &workspace);

    void updateWorkspaceNodeProperties(
            const EventSource &eventSrc,
            const int workspaceId, const WorkspaceNodePropertiesUpdate &update);

    //!
    //! \param workspaceId
    //! \param boardIds: boards of the workspace \e workspaceId
    //!
    void removeWorkspace(
            const EventSource &eventSrc, const int workspaceId, const QSet<int> &boardIds);

    void updateWorkspacesListProperties(
            const EventSource &eventSrc, const WorkspacesListPropertiesUpdate &propertiesUpdate);

    void createNewBoardWithId(
            const EventSource &eventSrc, const int boardId, const Board &board, const int workspaceId);

    void updateBoardNodeProperties(
            const EventSource &eventSrc,
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate);

    void removeBoard(const EventSource &eventSrc, const int boardId);

    void updateNodeRectProperties(
            const EventSource &eventSrc,
            const int boardId, const int cardId, const NodeRectDataUpdate &update);

    void createNodeRect(
            const EventSource &eventSrc,
            const int boardId, const int cardId, const NodeRectData &nodeRectData);

    void removeNodeRect(const EventSource &eventSrc, const int boardId, const int cardId);

    void createDataViewBox(
            const EventSource &eventSrc,
            const int boardId, const int customDataQueryId, const DataViewBoxData &dataViewBoxData);

    void updateDataViewBoxProperties(
            const EventSource &eventSrc,
            const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update);

    void removeDataViewBox(
            const EventSource &eventSrc, const int boardId, const int customDataQueryId);

    void createTopLevelGroupBoxWithId(
            const EventSource &eventSrc,
            const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData);

    void updateGroupBoxProperties(
            const EventSource &eventSrc,
            const int groupBoxId, const GroupBoxNodePropertiesUpdate &update);

    void removeGroupBoxAndReparentChildItems(const EventSource &eventSrc, const int groupBoxId);

    void removeNodeRectFromGroupBox(
            const EventSource &eventSrc, const int cardId, const int groupBoxId);

    //!
    //! The board having group-box `newParentGroupBox` must have the NodeRect for `cardId`.
    //!
    void addOrReparentNodeRectToGroupBox(
            const EventSource &eventSrc, const int cardId, const int newParentGroupBox);

    //!
    //! \param groupBoxId: must exist
    //! \param newParentGroupBoxId:
    //!           + if = -1: `groupBoxId` will be reparented to the board
    //!           + if != -1: must be on the same board as `groupBoxId`
    //!
    void reparentGroupBox(
            const EventSource &eventSrc, const int groupBoxId, const int newParentGroupBoxId);

    void updateMainWindowSize(const EventSource &eventSrc, const QSize &size);

    // ==== non-persisted independent data ====

    int getSingleHighlightedCardId() const override; // can return -1

    //!
    //! \param eventSrc
    //! \param cardId: -1 if no card or more than one card is highlighted
    //!
    void setSingleHighlightedCardId(
            const EventSource &eventSrc, const int cardId); // `cardId` can be -1

    void updateFontSizeScaleFactor(const QWidget *window, const double factor);

    // ==== derived data ====

private:
    PersistedDataAccess *persistedDataAccess;

    // non-persisted independent data
    int singleHighlightedCardId {-1};
            // If exactly one card is highlighted, this is set to that card.
            // Otherwise, this is set to -1.

    // derived data (can be cached or not)
    // - cache can be nullopt (not computed yet)
};

#endif // APPDATA_H
