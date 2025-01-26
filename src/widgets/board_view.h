#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <memory>
#include <QFrame>
#include <QGraphicsView>
#include <QMenu>
#include <QSet>
#include "models/board.h"
#include "models/custom_data_query.h"
#include "models/edge_arrow_data.h"
#include "models/group_box_tree.h"
#include "models/node_rect_data.h"
#include "models/relationship.h"
#include "models/relationships_bundle.h"
#include "models/settings/abstract_setting.h"
#include "widgets/common_types.h"
#include "widgets/icons.h"

class ActionDebouncer;
class BoardBoxItem;
class Card;
struct CardLabelToColorMapping;
class CardPropertiesUpdate;
class DataViewBox;
class EdgeArrow;
struct EdgeArrowData;
class GraphicsScene;
class GroupBox;
class NodeRect;
class SettingBox;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    //!
    //! Before calling this method:
    //!   + \c this must be visible
    //!   + \c canClose() must return true
    //! \param boardIdToLoad: if = -1, will only close the board
    //! \param callback: parameter \e highlightedCardIdChanged will be true if highlighted Card ID
    //!                  changed to -1
    //!
    void loadBoard(
            const int boardIdToLoad,
            std::function<void (bool loadOk, bool highlightedCardIdChanged)> callback);

    void prepareToClose();

    void applyZoomAction(const ZoomAction zoomAction);
    void toggleCardPreview();

    using LabelAndColor = std::pair<QString, QColor>;

    //!
    //! This can be called even if no board is loaded.
    //! \param cardLabelsAndAssociatedColors: in the order of precedence (high to low)
    //! \param defaultNodeRectColor
    //!
    void setColorsAssociatedWithLabels(
            const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &defaultNodeRectColor);

    void updateSettingBoxOnWorkspaceSetting(const int workspaceId, const SettingCategory category);

    //
    int getBoardId() const; // can be -1
    QPointF getViewTopLeftPos() const; // in canvas coordinates
    double getZoomRatio() const;

    QVector<LabelAndColor> getCardLabelsAndAssociatedColors() const {
        return cardLabelsAndAssociatedColors;
    }
    QColor getDefaultNodeRectColor() const {
        return defaultNodeRectColor;
    }

    bool canClose() const;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void workspaceCardLabelToColorMappingUpdatedViaSettingBox(
            const int workspaceId, const CardLabelToColorMapping &cardLabelToColorMapping);
    void hasWorkspaceSettingsPendingUpdateChanged(bool hasWorkspaceSettingsPendingUpdate);

private:
    static inline const QSizeF defaultNewNodeRectSize {200, 120};
    static inline const QSizeF defaultNewDataViewBoxSize {400, 400};
    static inline const QSizeF defaultNewSettingBoxSize {450, 600};
    static inline const QColor defaultNewDataViewBoxColor {170, 170, 170};
    static inline const QColor defaultNewSettingBoxColor {170, 170, 170};
    constexpr static double defaultEdgeArrowLineWidth {2.0};

    constexpr static double zValueForNodeRects {10.0};
    constexpr static double zValueForEdgeArrows {15.0};

    int boardId {-1}; // -1: no board loaded

    QVector<LabelAndColor> cardLabelsAndAssociatedColors; // in the order of precedence (high to low)
    QColor defaultNodeRectColor;

    double zoomScale {1.0};
    double graphicsGeometryScaleFactor {1.0};

    GroupBoxTree groupBoxTree;

    //
    QGraphicsView *graphicsView {nullptr};
    GraphicsScene *graphicsScene {nullptr};
    QGraphicsRectItem *canvas {nullptr}; // draw everything on this

    struct ContextMenu
    {
        explicit ContextMenu(BoardView *boardView);
        QMenu *menu;
        QPointF requestScenePos;

        void setActionIcons();
    private:
        BoardView *boardView;
        QHash<QAction *, Icon> actionToIcon;
    };
    ContextMenu contextMenu {this};

    ActionDebouncer *handleSettingsEditedDebouncer {nullptr};

    // setup
    void setUpWidgets();
    void setUpConnections();
    void installEventFiltersOnComponents();

    // event handlers
    void onUserToOpenExistingCard(const QPointF &scenePos);
    void onUserToCreateNewCard(const QPointF &scenePos);
    void onUserToDuplicateCard(const QPointF &scenePos);
    void onUserToCreateNewGroup(const QPointF &scenePos);
    void onUserToCreateNewCustomDataQuery(const QPointF &scenePos);
    void onUserToCreateSettingBox(const QPointF &scenePos);
    void onUserToCloseSettingBox(const SettingTargetTypeAndCategory &targetTypeAndCategory);
    void onUserToSetLabels(const int cardId);
    void onUserToCreateRelationship(const int cardId);
    void onUserToCloseNodeRect(const int cardId);
    void onUserToCloseDataViewBox(const int customDataQueryId);
    void onUserToSetGroupBoxTitle(const int groupBoxId, const QString &newTitle);
    void onUserToRemoveGroupBox(const int groupBoxId);
    void onBackgroundClicked();

    void reparentNodeRectInGroupBoxTree(const int cardId, const int newParentGroupBox);
            // - including adding/removing it to/from the tree
            // - `newParentGroupBox` = -1: remove it from tree
    void reparentGroupBoxInGroupBoxTree(const int groupBoxId, const int newParentGroupBox);
            //  - `newParentGroupBox` = -1: reparent it to the Board

    void handleSettingsEdited();

    //

    //!
    //! Remove all NodeRect's, EdgeArrow's, DataViewBox's, GroupBox's, etc. Does not check canClose().
    //!
    void closeAll(bool *highlightedCardIdChanged);

    //!
    //! Call this when
    //!   - graphicsView is resized,
    //!   - NodeRect or DataViewBox is added/moved/resized/removed,
    //!   - canvas's scale is set.
    //!
    void adjustSceneRect();

    //!
    //! \param zoomAction
    //! \param anchorScenePos: a point on the canvas at this position will be stationary relative
    //!                        to view
    //!
    void doApplyZoomAction(const ZoomAction zoomAction, const QPointF &anchorScenePos);

    void updateCanvasScale(const double scale, const QPointF &anchorScenePos);

    //
    class NodeRectsCollection
    {
    public:
        explicit NodeRectsCollection(BoardView *boardView_) : boardView(boardView_) {}

        //!
        //! The returned NodeRect is already added to canvas.
        //!
        NodeRect *createNodeRect(
                const int cardId, const Card &cardData, const QRectF &rect,
                const QColor &displayColor, const QColor &nodeRectOwnColor,
                const QStringList &userLabelsList);

        //!
        //! Does not check NodeRect::canClose().
        //! \param cardId
        //! \param removeConnectedEdgeArrows
        //! \param highlightedCardIdUpdated: will be true if highlighted Card ID changes to -1
        //!
        void closeNodeRect(
                const int cardId, const bool removeConnectedEdgeArrows,
                bool *highlightedCardIdUpdated);

        void setHighlightedCardIds(const QSet<int> &cardIdsToHighlight);
        QSet<int> addToHighlightedCards(const QSet<int> &cardIdsToHighlight);
                // returns all cards that are in highlighted state

        void updateAllNodeRectColors();
        void setAllNodeRectsTextEditorIgnoreWheelEvent(const bool b);

        bool contains(const int cardId) const;
        NodeRect *get(const int cardId) const;
        QSet<int> getAllCardIds() const;
        QColor getNodeRectOwnColor(const int cardId) const;
        QRectF getBoundingRectOfAllNodeRects() const; // returns QRectF() if no NodeRect exists

    private:
        BoardView *const boardView;
        QHash<int, NodeRect *> cardIdToNodeRect;
        QHash<int, QColor> cardIdToNodeRectOwnColor;
    };
    NodeRectsCollection nodeRectsCollection {this};

    //
    class RelationshipsCollection
    {
    public:
        explicit RelationshipsCollection(BoardView *boardView_) : boardView(boardView_) {}

        //!
        //! NodeRect's for start/end cards must exist.
        //! The returned EdgeArrow is already added to the scene.
        //!
        EdgeArrow *createEdgeArrow(
                const RelationshipId relId, const EdgeArrowData &edgeArrowData);

        //!
        //! \param relId
        //! \param updateOtherEdgeArrows: If = true, will also update the other EdgeArrows that
        //!                               connect the same pair of cards.
        //!
        void updateEdgeArrow(const RelationshipId &relId, const bool updateOtherEdgeArrows);

        void removeEdgeArrows(const QSet<RelationshipId> &relIds);

        void setAllEdgeArrowsVisible();
        void setLineColorAndLabelColorOfAllEdgeArrows(
                const QColor &lineColor, const QColor &labelColor);
        void hideEdgeArrows(const QSet<RelationshipId> &relIds);

        QSet<RelationshipId> getAllRelationshipIds() const;

    private:
        BoardView *const boardView;
        QHash<RelationshipId, EdgeArrow *> relIdToEdgeArrow;
        QHash<QSet<int>, QSet<RelationshipId>> cardIdPairToParallelRels;
                // "parallel relationships" := those connecting the same pair of cards

        void updateSingleEdgeArrow(
                const RelationshipId &relId, const int parallelIndex, const int parallelCount);
        static QVector<RelationshipId> sortRelationshipIds(const QSet<RelationshipId> relIds);
        QLineF computeEdgeArrowLine(
                const RelationshipId &relId, const int parallelIndex, const int parallelCount);
    };
    RelationshipsCollection relationshipsCollection {this};

    //
    class DataViewBoxesCollection
    {
    public:
        explicit DataViewBoxesCollection(BoardView *boardView_) : boardView(boardView_) {}

        //!
        //! The returned DataViewBox is already added to canvas.
        //!
        DataViewBox *createDataViewBox(
                const int customDataQueryId, const CustomDataQuery &customDataQueryData,
                const QRectF &rect, const QColor &displayColor, const QColor &dataViewBoxOwnColor);

        void closeDataViewBox(const int customDataQueryId);

        void setAllDataViewBoxesTextEditorIgnoreWheelEvent(const bool ignoreWheelEvent);

        bool contains(const int customDataQueryId) const;
        QSet<int> getAllCustomDataQueryIds() const;
        QRectF getBoundingRectOfAllDataViewBoxes() const;
                // returns QRectF() if no DataViewBox exists

    private:
        BoardView *const boardView;
        QHash<int, DataViewBox *> customDataQueryIdToDataViewBox;
        QHash<int, QColor> customDataQueryIdToDataViewBoxOwnColor;
    };
    DataViewBoxesCollection dataViewBoxesCollection {this};

    //
    class GroupBoxesCollection
    {
    public:
        explicit GroupBoxesCollection(BoardView *boardView) : boardView(boardView) {}

        GroupBox *createGroupBox(const int groupBoxId, const GroupBoxData &groupBoxData);
        void removeGroupBox(const int groupBoxId);

        void setHighlightedGroupBoxes(const QSet<int> &groupBoxIds);
        void addToHighlightedGroupBoxes(const QSet<int> &groupBoxIds);
        void unhighlightGroupBoxes(const QSet<int> &groupBoxIds);

        void setColorOfAllGroupBoxes(const QColor &color);

        GroupBox *get(const int groupBoxId); // returns nullptr if not found
        QSet<int> getAllGroupBoxIds() const;

        //!
        //! Returns the deepest (smallest) of the group-boxes enclosing `boardBoxItem` (not
        //! considering `groupBoxIdsToExclude`), if they form a single set of nesting group-boxes.
        //! Otherwise, returns \c nullopt.
        //!
        std::optional<int> getDeepestEnclosingGroupBox(
                const BoardBoxItem *boardBoxItem,
                const QSet<int> &groupBoxIdsToExclude = QSet<int> {});

    private:
        BoardView *const boardView;
        QHash<int, GroupBox *> groupBoxes;

        void highlightGroupBoxAndDescendants(
                const int groupBoxIdToHighlight, const bool unhlighlightOtherItems);
    };
    GroupBoxesCollection groupBoxesCollection {this};

    //
    class RelationshipBundlesCollection
    {
    public:
        explicit RelationshipBundlesCollection(BoardView *boardView) : boardView(boardView) {}

        //!
        //! Updates the whole collection. This can be called when a relationship is created/removed,
        //! a NodeRect is created/removed, or the GroupBoxTree is modified.
        //!
        void update();

        void updateBundlesConnectingGroupBox(const int groupBoxId);
        void updateBundlesConnectingNodeRect(const int cardId);

        //
        QSet<RelationshipsBundle> getBundlesOfGroupBox(const int groupBoxId) const;
        QSet<RelationshipId> getBundledRelationships() const;

        //
        void setLineColorAndLabelColorOfAllEdgeArrows(
                const QColor &lineColor, const QColor &labelColor);

    private:
        BoardView *const boardView;

        // data
        QHash<int, QSet<RelationshipsBundle>> relBundlesByGroupBoxId;
        QHash<int, QSet<RelationshipsBundle>> relBundlesByExternalCardId;
        QSet<RelationshipId> bundledRels;

        using GroupBoxAndCard = std::pair<int, int>;
        QHash<GroupBoxAndCard, QSet<RelationshipsBundle>> relBundlesByGroupBoxAndCard;
                // relBundlesByGroupBoxAndCard[(g, c)] is the bundles connecting g & c

        // EdgeArrow's
        QHash<RelationshipsBundle, EdgeArrow *> relBundleToEdgeArrow;
        void redrawBundleArrows();
        void updateEdgeArrow(
                const RelationshipsBundle &bundle, const int parallelIndex, const int parallelCount);

        //
        static QHash<RelationshipsBundle, int> computeBundleToParallelIndex(
                const QHash<int, QSet<RelationshipsBundle>> &relBundlesByGroupBoxId);
        QLineF computeEdgeArrowLine(
                const RelationshipsBundle &bundle, const int parallelIndex, const int parallelCount);


    };
    RelationshipBundlesCollection relationshipBundlesCollection {this};

    void updateRelationshipBundles();

    //
    class SettingBoxesCollection
    {
    public:
        explicit SettingBoxesCollection(BoardView *boardView);

        //!
        //! The setting box for (targetType, category) must not already exist.
        //! The resulting SettingBox is already added to canvas.
        //! The result will be \c nullptr if failed.
        //!
        void createSettingBox(
                const SettingTargetTypeAndCategory targetTypeAndCategory,
                const QRectF &rect, const QColor &displayColor,
                std::function<void (SettingBox *result)> callback);
        void closeSettingBox(const SettingTargetTypeAndCategory targetTypeAndCategory);
        void updateSettingBoxIfExists(
                const SettingTargetTypeAndCategory targetTypeAndCategory,
                const int targetId);
        void updateAllSettingBoxesColors();
        void setAllSettingBoxesTextEditorIgnoreWheelEvent(const bool b);
        void handleEditedSettings(); // consumes `editedSettings`

        QSet<SettingTargetTypeAndCategory> getAllSettingBoxes() const;
        bool containsSetting(const SettingTargetTypeAndCategory targetTypeAndCategory) const;

    private:
        BoardView *const boardView;
        using AbstractSetting = AbstractWorkspaceOrBoardSetting;
        struct SettingsBoxAndData
        {
            SettingBox *box;
            std::shared_ptr<AbstractSetting> data;
            int targetId;
        };
        QHash<SettingTargetTypeAndCategory, SettingsBoxAndData> settingBoxes;
        QSet<SettingTargetTypeAndCategory> editedSettings;

        void createSettingDataForWorkspace(
                const int workspaceId, const SettingCategory category,
                std::function<void (AbstractSetting *setting)> callback); // `setting` be nullptr
        AbstractSetting *createSettingDataForBoard(
                const SettingCategory category); // can return nullptr

        void applyWorkspaceSetting(
                const int workspaceId, std::shared_ptr<AbstractSetting> settingData);
        void applyBoardSetting(
                const int boardId, std::shared_ptr<AbstractSetting> settingData);
    };
    SettingBoxesCollection settingBoxesCollection {this};

    // item moving/resizing state
    struct ItemMovingResizingStateData
    {
        void activateWithTargetNodeRect(const int cardId) {
            clear(); targetCardId = cardId;
        }
        void activateWithTargetGroupBox(
                const int groupBoxId,
                const QSet<int> &descendantGroupBoxes, const QSet<int> &descendantCards) {
            clear();
            targetGroupBoxId = groupBoxId;
            descendantGroupBoxesOfTargetGroupBox = descendantGroupBoxes;
            descendantCardsOfTargetGroupBox = descendantCards;
        }
        void deactivate() { clear(); }

        bool targetIsNodeRect(const int cardId) const { return targetCardId == cardId; }
        bool targetIsGroupBox(const int groupBoxId) const { return targetGroupBoxId == groupBoxId; }

        // parameters to be used when moving/resizing finishes
        std::optional<int> newParentGroupBoxId;
                // + if != -1: reparent the target to this group-box
                // + if == -1: reparent to board (if target is a group-box) or remove from
                //             original group-box (if target is a NodeRect)
                // + Note that it can be the target's original parent.

        // other parameters
        QSet<int> descendantGroupBoxesOfTargetGroupBox;
        QSet<int> descendantCardsOfTargetGroupBox;

    private:
        int targetCardId {-1}; // when the item being moved/resized is a NodeRect
        int targetGroupBoxId {-1}; // when the item being moved/resized is a group-box

        void clear() {
            targetCardId = -1;
            targetGroupBoxId = -1;
            newParentGroupBoxId = std::nullopt;
            descendantGroupBoxesOfTargetGroupBox.clear();
            descendantCardsOfTargetGroupBox.clear();
        }
    };
    ItemMovingResizingStateData itemMovingResizingStateData;

    // co-moving state
    struct ComovingStateData
    {
        bool getIsActive() const { return isActive; }

        QPointF followeeInitialPos;
        QHash<int, QPointF> followerGroupBoxIdToInitialPos;
        QHash<int, QPointF> followerCardIdToInitialPos; // (initial positions of NodeRect's)

        //
        void activate();
        void deactivate();

        void clearFollowers() {
            followerGroupBoxIdToInitialPos.clear();
            followerCardIdToInitialPos.clear();
        }
    private:
        bool isActive {false};

    };
    ComovingStateData comovingStateData;

    void moveFollowerItemsInComovingState(
            const QPointF &displacement, const ComovingStateData &comovingStateData);
            // does not save to AppData
    void savePositionsOfComovingItems(const ComovingStateData &comovingStateData);

    // tools
    void getWorkspaceId(std::function<void (const int workspaceId)> callback);
            // `workspaceId` can be -1

    static QColor getSceneBackgroundColor(const bool isDarkTheme);
    QColor getEdgeArrowLineColor() const; // calls AppDataReadonly
    QColor getEdgeArrowLabelColor() const; // calls AppDataReadonly
    static QColor computeGroupBoxColor(const bool isDarkTheme);

    QPoint getScreenPosFromScenePos(const QPointF &scenePos) const;
    QPointF getViewCenterInScene() const;
    void setViewTopLeftPos(const QPointF &canvasPos);
    void moveSceneRelativeToView(const QPointF &displacement); // displacement: in pixel

    static QColor computeNodeRectDisplayColor(
            const QColor &nodeRectOwnColor,
            const QSet<QString> &cardLabels,
            const QVector<LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &boardDefaultColorForNodeRect,
            const bool invertLightness);
    static QColor computeDataViewBoxDisplayColor(
            const QColor &dataViewBoxOwnColor, const QColor &boardDefaultColorForDataViewBox);
    static QColor computeSettingBoxDisplayColor(
            const QColor &boardDefaultColorForSettingBox, const bool invertLightness);

    QSet<RelationshipId> getEdgeArrowsConnectingNodeRect(const int cardId);

    static QLineF computeArrowLineConnectingRects(
            const QRectF &fromRect, const QRectF &toRect,
            const int parallelIndex, const int parallelCount);
};

#endif // BOARDVIEW_H
