#include "app_data.h"
#include "persisted_data_access.h"
#include "utilities/message_box.h"

AppData::AppData(PersistedDataAccess *persistedDataAccess_, QObject *parent)
        : AppDataReadonly(parent)
        , persistedDataAccess(persistedDataAccess_) {
}

void AppData::queryCards(
        const QSet<int> &cardIds, std::function<void (bool, const QHash<int, Card> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryCards(cardIds, callback, callbackContext);
}

void AppData::queryRelationship(
        const RelId &relationshipId,
        std::function<void (bool, const std::optional<RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryRelationship(relationshipId, callback, callbackContext);
}

void AppData::queryRelationshipsFromToCards(
        const QSet<int> &cardIds,
        std::function<void (bool, const QHash<RelId, RelProperties> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryRelationshipsFromToCards(cardIds, callback, callbackContext);
}

void AppData::getUserLabelsAndRelationshipTypes(
        std::function<void (bool, const StringListPair &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getUserLabelsAndRelationshipTypes(callback, callbackContext);
}

void AppData::requestNewCardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    persistedDataAccess->requestNewCardId(callback, callbackContext);
}

void AppData::getWorkspaces(
        std::function<void (bool, const QHash<int, Workspace> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getWorkspaces(callback, callbackContext);
}

void AppData::getWorkspacesListProperties(
        std::function<void (bool, WorkspacesListProperties)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getWorkspacesListProperties(callback, callbackContext);
}

void AppData::getBoardIdsAndNames(
        std::function<void (bool, const QHash<int, QString> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getBoardIdsAndNames(callback, callbackContext);
}

void AppData::getBoardData(
        const int boardId, std::function<void (bool, std::optional<Board>)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->getBoardData(boardId, callback, callbackContext);
}

void AppData::requestNewBoardId(
        std::function<void (std::optional<int>)> callback, QPointer<QObject> callbackContext) {
    persistedDataAccess->requestNewBoardId(callback, callbackContext);
}

void AppData::queryCustomDataQueries(
        const QSet<int> &customDataQueryIds,
        std::function<void (bool, const QHash<int, CustomDataQuery> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->queryCustomDataQueries(customDataQueryIds, callback, callbackContext);
}

void AppData::performCustomCypherQuery(
        const QString &cypher, const QJsonObject &parameters,
        std::function<void (bool, const QVector<QJsonObject> &)> callback,
        QPointer<QObject> callbackContext) {
    persistedDataAccess->performCustomCypherQuery(cypher, parameters, callback, callbackContext);
}

std::optional<QRect> AppData::getMainWindowSizePos() {
    return persistedDataAccess->getMainWindowSizePos();
}

bool AppData::getIsDarkTheme() {
    return persistedDataAccess->getIsDarkTheme();
}

bool AppData::getAutoAdjustCardColorsForDarkTheme() {
    return persistedDataAccess->getAutoAdjustCardColorsForDarkTheme();
}

QString AppData::getExportOutputDir(){
    return persistedDataAccess->getExportOutputDir();
}

void AppData::createNewCardWithId(
        const EventSource &/*eventSrc*/, const int cardId, const Card &card) {
    // 1. persist
    persistedDataAccess->createNewCardWithId(cardId, card);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateCardProperties(
        const EventSource &eventSrc, const int cardId,
        const CardPropertiesUpdate &cardPropertiesUpdate) {
    // 1. persist
    persistedDataAccess->updateCardProperties(cardId, cardPropertiesUpdate);

    // 2. update all variables and emit "updated" signals
    emit cardPropertiesUpdated(eventSrc, cardId, cardPropertiesUpdate);
}

void AppData::updateCardLabels(
        const EventSource &/*eventSrc*/, const int cardId, const QSet<QString> &updatedLabels) {
    // 1. persist
    persistedDataAccess->updateCardLabels(cardId, updatedLabels);

    // 2. update all variables and emit "updated" signals
}

void AppData::createNewCustomDataQueryWithId(
        const EventSource &/*eventSrc*/,
        const int customDataQueryId, const CustomDataQuery &customDataQuery) {
    // 1. persist
    persistedDataAccess->createNewCustomDataQueryWithId(customDataQueryId, customDataQuery);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateCustomDataQueryProperties(
        const EventSource &eventSrc,
        const int customDataQueryId, const CustomDataQueryUpdate &update) {
    // 1. persist
    persistedDataAccess->updateCustomDataQueryProperties(customDataQueryId, update);

    // 2. update all variables and emit "updated" signals
    emit customDataQueryUpdated(eventSrc, customDataQueryId, update);
}

void AppData::createRelationship(const EventSource &/*eventSrc*/, const RelationshipId &id) {
    // 1. persist
    persistedDataAccess->createRelationship(id);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateUserRelationshipTypes(
        const EventSource &/*eventSrc*/, const QStringList &updatedRelTypes)  {
    // 1. persist
    persistedDataAccess->updateUserRelationshipTypes(updatedRelTypes);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateUserCardLabels(
        const EventSource &/*eventSrc*/, const QStringList &updatedCardLabels) {
    // 1. persist
    persistedDataAccess->updateUserCardLabels(updatedCardLabels);

    // 2. update all variables and emit "updated" signals
}

void AppData::createNewWorkspaceWithId(
        const EventSource &/*eventSrc*/, const int workspaceId, const Workspace &workspace) {
    // 1. persist
    persistedDataAccess->createNewWorkspaceWithId(workspaceId, workspace);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateWorkspaceNodeProperties(
        const EventSource &/*eventSrc*/,
        const int workspaceId, const WorkspaceNodePropertiesUpdate &update) {
    // 1. persist
    persistedDataAccess->updateWorkspaceNodeProperties(workspaceId, update);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeWorkspace(
        const EventSource &/*eventSrc*/, const int workspaceId, const QSet<int> &boardIds) {
    // 1. persist
    persistedDataAccess->removeWorkspace(workspaceId, boardIds);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateWorkspacesListProperties(
        const EventSource &/*eventSrc*/, const WorkspacesListPropertiesUpdate &propertiesUpdate) {
    // 1. persist
    persistedDataAccess->updateWorkspacesListProperties(propertiesUpdate);

    // 2. update all variables and emit "updated" signals
}

void AppData::createNewBoardWithId(
        const EventSource &/*eventSrc*/, const int boardId, const Board &board, const int workspaceId) {
    // 1. persist
    persistedDataAccess->createNewBoardWithId(boardId, board, workspaceId);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateBoardNodeProperties(
        const EventSource &/*eventSrc*/, const int boardId,
        const BoardNodePropertiesUpdate &propertiesUpdate) {
    // 1. persist
    persistedDataAccess->updateBoardNodeProperties(boardId, propertiesUpdate);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeBoard(const EventSource &/*eventSrc*/, const int boardId) {
    // 1. persist
    persistedDataAccess->removeBoard(boardId);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateNodeRectProperties(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectDataUpdate &update) {
    // 1. persist
    persistedDataAccess->updateNodeRectProperties(boardId, cardId, update);

    // 2. update all variables and emit "updated" signals
}

void AppData::createNodeRect(
        const EventSource &/*eventSrc*/,
        const int boardId, const int cardId, const NodeRectData &nodeRectData) {
    // 1. persist
    persistedDataAccess->createNodeRect(boardId, cardId, nodeRectData);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeNodeRect(
        const EventSource &/*eventSrc*/, const int boardId, const int cardId) {
    // 1. persist
    persistedDataAccess->removeNodeRect(boardId, cardId);

    // 2. update all variables and emit "updated" signals
}

void AppData::createDataViewBox(
        const EventSource &/*eventSrc*/, const int boardId, const int customDataQueryId,
        const DataViewBoxData &dataViewBoxData) {
    // 1. persist
    persistedDataAccess->createDataViewBox(boardId, customDataQueryId, dataViewBoxData);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateDataViewBoxProperties(
        const EventSource &/*eventSrc*/,
        const int boardId, const int customDataQueryId, const DataViewBoxDataUpdate &update) {
    // 1. persist
    persistedDataAccess->updateDataViewBoxProperties(boardId, customDataQueryId, update);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeDataViewBox(
        const EventSource &/*eventSrc*/, const int boardId, const int customDataQueryId) {
    // 1. persist
    persistedDataAccess->removeDataViewBox(boardId, customDataQueryId);

    // 2. update all variables and emit "updated" signals
}

void AppData::createTopLevelGroupBoxWithId(
        const EventSource &/*eventSrc*/,
        const int boardId, const int groupBoxId, const GroupBoxData &groupBoxData) {
    // 1. persist
    persistedDataAccess->createTopLevelGroupBoxWithId(boardId, groupBoxId, groupBoxData);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateGroupBoxProperties(
        const EventSource &/*eventSrc*/, const int groupBoxId, const GroupBoxNodePropertiesUpdate &update) {
    // 1. persist
    persistedDataAccess->updateGroupBoxProperties(groupBoxId, update);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeGroupBoxAndReparentChildItems(
        const EventSource &/*eventSrc*/, const int groupBoxId) {
    // 1. persist
    persistedDataAccess->removeGroupBoxAndReparentChildItems(groupBoxId);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeNodeRectFromGroupBox(
        const EventSource &/*eventSrc*/, const int cardId, const int groupBoxId) {
    // 1. persist
    persistedDataAccess->removeNodeRectFromGroupBox(cardId, groupBoxId);

    // 2. update all variables and emit "updated" signals
}

void AppData::addOrReparentNodeRectToGroupBox(
        const EventSource &/*eventSrc*/, const int cardId, const int newParentGroupBox) {
    // 1. persist
    persistedDataAccess->addOrReparentNodeRectToGroupBox(cardId, newParentGroupBox);

    // 2. update all variables and emit "updated" signals
}

void AppData::reparentGroupBox(
        const EventSource &/*eventSrc*/, const int groupBoxId, const int newParentGroupBoxId) {
    // 1. persist
    persistedDataAccess->reparentGroupBox(groupBoxId, newParentGroupBoxId);

    // 2. update all variables and emit "updated" signals
}

void AppData::createSettingBox(
        const EventSource &/*eventSrc*/, const int boardId, const SettingBoxData &settingBoxData) {
    // 1. persist
    persistedDataAccess->createSettingBox(boardId, settingBoxData);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateSettingBoxProperties(
        const EventSource &/*eventSrc*/, const int boardId, const SettingTargetType targetType,
        const SettingCategory category, const SettingBoxDataUpdate &update) {
    // 1. persist
    persistedDataAccess->updateSettingBoxProperties(boardId, targetType, category, update);

    // 2. update all variables and emit "updated" signals
}

void AppData::removeSettingBox(
        const EventSource &/*eventSrc*/, const int boardId, const SettingTargetType targetType,
        const SettingCategory category) {
    // 1. persist
    persistedDataAccess->removeSettingBox(boardId, targetType, category);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateMainWindowSizePos(const EventSource &/*eventSrc*/, const QRect &rect) {
    // 1. persist
    persistedDataAccess->saveMainWindowSizePos(rect);

    // 2. update all variables and emit "updated" signals
}

void AppData::updateIsDarkTheme(const EventSource &/*eventSrc*/, const bool isDarkTheme) {
    // 1. persist
    persistedDataAccess->saveIsDarkTheme(isDarkTheme);

    // 2. update all variables and emit "updated" signals
    emit isDarkThemeUpdated(isDarkTheme);
}

void AppData::updateAutoAdjustCardColorsForDarkTheme(
        const EventSource &/*eventSrc*/, const bool autoAdjust) {
    // 1. persist
    persistedDataAccess->saveAutoAdjustCardColorsForDarkTheme(autoAdjust);

    // 2. update all variables and emit "updated" signals
    emit autoAdjustCardColorsForDarkThemeUpdated(autoAdjust);
}

void AppData::updateExportOutputDir(const EventSource &/*eventSrc*/, const QString &outputDir) {
    // 1. persist
    persistedDataAccess->saveExportOutputDir(outputDir);

    // 2. update all variables and emit "updated" signals
}

int AppData::getSingleHighlightedCardId() const {
    return singleHighlightedCardId;
}

double AppData::getFontSizeScaleFactor(const QWidget *window) const {
    return windowToFontSizeScaleFactor.value(window, 1.0);
}

void AppData::setSingleHighlightedCardId(const EventSource &eventSrc, const int cardId) {
    // synchornously update derived variables and emit "updated" signals
    singleHighlightedCardId = cardId;
    emit highlightedCardIdUpdated(eventSrc);
}

void AppData::updateFontSizeScaleFactor(const QWidget *window, const double factor) {
    windowToFontSizeScaleFactor[window] = factor;

    // synchornously update derived variables and emit "updated" signals
    emit fontSizeScaleFactorChanged(window, factor);
}
