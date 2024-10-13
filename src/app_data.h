#ifndef APPDATA_H
#define APPDATA_H

#include <QObject>
#include "app_data_readonly.h"
#include "app_event_source.h"
#include "models/board.h"
#include "models/boards_list_properties.h"
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

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext) override;

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) override;

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext) override;

    std::optional<QSize> getMainWindowSize() override;

    // ---- persisted data: update ----

    // If persistence fails, a record of unsaved update is added and a message box is shown.

    void createNewCardWithId(const EventSource &eventSrc, const int cardId, const Card &card);

    void updateCardProperties(
            const EventSource &eventSrc,
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);

    void updateCardLabels(
            const EventSource &eventSrc, const int cardId, const QSet<QString> &updatedLabels);

    //!
    //! The start/end cards must already exist (which is not checked here).
    //! It's not an error if the relationship already exists.
    //!
    void createRelationship(const EventSource &eventSrc, const RelationshipId &id);

    void updateUserRelationshipTypes(
            const EventSource &eventSrc, const QStringList &updatedRelTypes);

    void updateUserCardLabels(
            const EventSource &eventSrc, const QStringList &updatedCardLabels);

    void updateBoardsListProperties(
            const EventSource &eventSrc, const BoardsListPropertiesUpdate &propertiesUpdate);

    void createNewBoardWithId(
            const EventSource &eventSrc, const int boardId, const Board &board);

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

    void updateMainWindowSize(const EventSource &eventSrc, const QSize &size);

    // ==== non-persisted independent data ====

    int getHighlightedCardId() const override; // can return -1

    void setHighlightedCardId(const EventSource &eventSrc, const int cardId); // `cardId` can be -1

    // ==== derived data ====

private:
    PersistedDataAccess *persistedDataAccess;

    // non-persisted independent data
    int highlightedCardId {-1};

    // derived data (can be cached or not)
    // - cache can be nullopt (not computed yet)
};

#endif // APPDATA_H
