#ifndef APP_DATA_READONLY_H
#define APP_DATA_READONLY_H

#include <QWidget>
#include "app_event_source.h"
#include "models/board.h"
#include "models/card.h"
#include "models/custom_data_query.h"
#include "models/relationship.h"
#include "models/workspace.h"
#include "models/workspaces_list_properties.h"

class AppDataReadonly : public QObject
{
    Q_OBJECT
public:
    explicit AppDataReadonly(QObject *parent = nullptr);

    // ==== persisted data ====

    virtual void queryCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<int, Card> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    using RelId = RelationshipId;
    using RelProperties = RelationshipProperties;

    virtual void queryRelationship(
            const RelId &relationshipId,
            std::function<void (bool ok, const std::optional<RelProperties> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void queryRelationshipsFromToCards(
            const QSet<int> &cardIds,
            std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
            QPointer<QObject> callbackContext) = 0;

    using StringListPair = std::pair<QStringList, QStringList>;
    virtual void getUserLabelsAndRelationshipTypes(
            std::function<void (bool ok, const StringListPair &labelsAndRelTypes)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void requestNewCardId(
            std::function<void (std::optional<int> cardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getWorkspaces(
            std::function<void (bool ok, const QHash<int, Workspace> &workspaces)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getWorkspacesListProperties(
            std::function<void (bool ok, WorkspacesListProperties properties)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardIdsAndNames(
            std::function<void (bool ok, const QHash<int, QString> &idToName)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void getBoardData(
            const int boardId,
            std::function<void (bool ok, std::optional<Board> board)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void requestNewBoardId(
            std::function<void (std::optional<int> boardId)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void queryCustomDataQueries(
            const QSet<int> &customDataQueryIds,
            std::function<void (bool ok, const QHash<int, CustomDataQuery> &dataQueries)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual void performCustomCypherQuery(
            const QString &cypher, const QJsonObject &parameters,
            std::function<void (bool ok, const QVector<QJsonObject> &result)> callback,
            QPointer<QObject> callbackContext) = 0;

    virtual std::optional<QSize> getMainWindowSize() = 0;

    virtual bool getIsDarkTheme() = 0;

    // ==== non-persisted independent data ====

    //!
    //! \return -1 if no card or more than one card is highlighted
    //!
    virtual int getSingleHighlightedCardId() const = 0; // can return -1

signals:
    void cardPropertiesUpdated(
            EventSource eventSrc,
            const int cardId, const CardPropertiesUpdate &cardPropertiesUpdate);
    void customDataQueryUpdated(
            EventSource eventSrc,
            const int customDataQueryId, const CustomDataQueryUpdate &update);
    void highlightedCardIdUpdated(EventSource eventSrc);
    void fontSizeScaleFactorChanged(const QWidget *window, const double factor);
    void isDarkThemeUpdated(const bool isDarkTheme);
};

#endif // APP_DATA_READONLY_H
