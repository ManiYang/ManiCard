#ifndef APPDATA_H
#define APPDATA_H

#include <QObject>
#include "app_event.h"
#include "models/board.h"
#include "models/boards_list_properties.h"
#include "models/card.h"
#include "models/relationship.h"

class PersistedDataAccess;

//!
//! independent data (persisted, non-persisted), derived data
//!
//! get
//!   - can be asynchronous
//!
//! update
//!   1. synchornously update all variables and emit "updated" signals
//!   2. persist (if needed)
//!
class AppData : public QObject
{
    Q_OBJECT
public:
    explicit AppData(PersistedDataAccess *persistedDataAccess_, QObject *parent = nullptr);

    // ==== persisted data ====

    // ---- persisted data: get ----

    void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext);

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

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

    void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext);

    void getBoardsListProperties(
            std::function<void (bool ok, BoardsListProperties properties)> callback,
            QPointer<QObject> callbackContext);

    void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext);

    void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext);

    std::optional<QSize> getMainWindowSize();


    // ---- persisted data: update ----

    // If persistence fails, a record of unsaved update is added.

    void createNewCardWithId(
            const int cardId, const Card &card,
            std::function<void (bool ok)> callbackPersistResult, QPointer<QObject> callbackContext);

    void updateCardProperties(
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateCardLabels(
            const int cardId, const QSet<QString> &updatedLabels,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    //!
    //! The start/end cards must already exist (which is not checked here).
    //! It's not an error if the relationship already exists.
    //!
    void createRelationship(
            const RelationshipId &id, std::function<void (bool ok, bool created)> callback,
            QPointer<QObject> callbackContext);

    void updateUserRelationshipTypes(
            const QStringList &updatedRelTypes, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext);

    void updateUserCardLabels(
            const QStringList &updatedCardLabels, std::function<void (bool ok)> callback,
            QPointer<QObject> callbackContext);

    void updateBoardsListProperties(
            const BoardsListPropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void createNewBoardWithId(
            const int boardId, const Board &board,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateBoardNodeProperties(
            const int boardId, const BoardNodePropertiesUpdate &propertiesUpdate,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void removeBoard(
            const int boardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void updateNodeRectProperties(
            const int boardId, const int cardId, const NodeRectDataUpdate &update,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void createNodeRect(
            const int boardId, const int cardId, const NodeRectData &nodeRectData,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    void removeNodeRect(
            const int boardId, const int cardId,
            std::function<void (bool ok)> callback, QPointer<QObject> callbackContext);

    bool saveMainWindowSize(const QSize &size);

    // ==== non-persisted independent data ====

    int getHighlightedCardId() const; // can return -1

    void setHighlightedCardId(const int cardId, EventSource eventSrc); // `cardId` can be -1


    // ==== derived data ====



signals:
    void highlightedCardIdUpdated(EventSource eventSrc);

private:
    PersistedDataAccess *persistedDataAccess;

    // non-persisted independent data
    int highlightedCardId {-1};

    // derived data (can be cached or not)
    // - cache can be nullopt (not computed yet)

};

#endif // APPDATA_H
