#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>
#include "models/edge_arrow_data.h"
#include "models/node_rect_data.h"
#include "models/relationship.h"

class BoardViewToolBar;
class EdgeArrow;
struct EdgeArrowData;
class Card;
class CardPropertiesUpdate;
class GraphicsScene;
class NodeRect;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    //!
    //! Before calling this method:
    //!   + \c this must be visible
    //!   + \c canClose() must returns true
    //! \param boardId_: if = -1, will only close the board
    //! \param callback
    //!
    void loadBoard(const int boardIdToLoad, std::function<void (bool ok)> callback);

    void prepareToClose();

    //
    void rightSideBarClosed();

    //
    int getBoardId() const; // can be -1
    QPointF getViewTopLeftPos() const;

    bool canClose() const;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void openRightSideBar();

private:
    static inline const QSizeF defaultNewNodeRectSize {200, 120};
    static inline const QColor defaultNewNodeRectColor {170, 170, 170};
    static inline const QColor defaultEdgeArrowLineColor {100, 100, 100};
    constexpr static double defaultEdgeArrowLineWidth {2.0};

    constexpr static double zValueForNodeRects {10.0};
    constexpr static double zValueForEdgeArrows {15.0};

    int boardId {-1}; // -1: no board loaded

    //
    BoardViewToolBar *toolBar {nullptr};
    QGraphicsView *graphicsView {nullptr};
    GraphicsScene *graphicsScene {nullptr};

    struct ContextMenuData
    {
        QPointF requestScenePos;
    };
    QMenu *contextMenu;
    ContextMenuData contextMenuData;

    // setup
    void setUpWidgets();
    void setUpContextMenu();
    void setUpConnections();
    void installEventFiltersOnComponents();

    //

    //!
    //! Call this whenever graphicsView is resized, or graphics items are added/removed.
    //!
    void adjustSceneRect();

    void userToOpenExistingCard(const QPointF &scenePos);
    void userToCreateNewCard(const QPointF &scenePos);
    void userToSetLabels(const int cardId);
    void userToCreateRelationship(const int cardId);
    void userToCloseNodeRect(const int cardId);

    void openExistingCard(const int cardId, const QPointF &scenePos);
    void saveCardPropertiesUpdate(
            NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
            std::function<void ()> callback); // callback will be called in context of `this`

    void onHighlightedCardChanged(const int cardId); // `cardId` can be -1

    //!
    //! Remove all NodeRect's and EdgeArrow's. Does not check canClose().
    //!
    void closeAllCards();

    //
    class NodeRectsCollection
    {
    public:
        explicit NodeRectsCollection(BoardView *boardView_) : boardView(boardView_) {}

        //!
        //! The returned NodeRect is already added to the scene.
        //!
        NodeRect *createNodeRect(
                const int cardId, const Card &cardData,
                const NodeRectData &nodeRectData, const bool saveCreatedNodeRectData,
                const QStringList &userLabelsList);

        //!
        //! Does not check NodeRect::canClose().
        //!
        void closeNodeRect(const int cardId, const bool removeConnectedEdgeArrows);

        void unhighlightAllCards();

        bool contains(const int cardId) const;
        NodeRect *get(const int cardId) const;
        QSet<int> getAllCardIds() const;
        QSet<NodeRect *> getAllNodeRects() const;

    private:
        BoardView *boardView;
        QHash<int, NodeRect *> cardIdToNodeRect;
        int highlightedCardId {-1};
    };
    NodeRectsCollection nodeRectsCollection {this};

    //
    class EdgeArrowsCollection
    {
    public:
        explicit EdgeArrowsCollection(BoardView *boardView_) : boardView(boardView_) {}

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

        QSet<RelationshipId> getAllRelationshipIds() const;

    private:
        BoardView *boardView;
        QHash<RelationshipId, EdgeArrow *> relIdToEdgeArrow;
        QHash<QSet<int>, QSet<RelationshipId>> cardIdPairToParallelRels;
                // "parallel relationships" := those connecting the same pair of cards

        void updateSingleEdgeArrow(
                const RelationshipId &relId, const int parallelIndex, const int parallelCount);
        static QVector<RelationshipId> sortRelationshipIds(const QSet<RelationshipId> relIds);
        QLineF computeEdgeArrowLine(
                const RelationshipId &relId, const int parallelIndex, const int parallelCount);
    };
    EdgeArrowsCollection edgeArrowsCollection {this};

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);
    void setViewTopLeftPos(const QPointF &scenePos);

    QSet<RelationshipId> getEdgeArrowsConnectingNodeRect(const int cardId);
};

#endif // BOARDVIEW_H
