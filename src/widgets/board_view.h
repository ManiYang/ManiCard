#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>
#include "models/edge_arrow_data.h"
#include "models/node_rect_data.h"
#include "models/relationship.h"

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
    int getBoardId() const; // can be -1
    QPointF getViewTopLeftPos() const;

    bool canClose() const;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    static inline const QSizeF defaultNewNodeRectSize {200, 120};
    static inline const QColor defaultNewNodeRectColor {170, 170, 170};
    static inline const QColor defaultNewEdgeArrowLineColor {100, 100, 100};
    constexpr static double defaultNewEdgeArrowLineWidth {2.0};

    constexpr static double zValueForNodeRects {10.0};
    constexpr static double zValueForEdgeArrows {15.0};

    int boardId {-1}; // -1: no board loaded
    QHash<int, NodeRect *> cardIdToNodeRect;
            // currently opened cards. Updated in createNodeRect() & closeNodeRect().
    QHash<RelationshipId, EdgeArrow *> relIdToEdgeArrow;
            // updated in createEdgeArrow() & removeEdgeArrow()

    // component widgets:
    QGraphicsView *graphicsView {nullptr};

    //
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
    void userToCreateRelationship(const int cardId);
    void userToCloseNodeRect(const int cardId);
    void openExistingCard(const int cardId, const QPointF &scenePos);
    void saveCardPropertiesUpdate(
            NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
            std::function<void ()> callback); // callback will be called in context of `this`

    //!
    //! Remove all NodeRect's and EdgeArrow's.
    //! Does not check canClose().
    //!
    void closeAllCards();

    // node rect creation and removal

    //!
    //! The returned NodeRect is already added to the scene.
    //!
    NodeRect *createNodeRect(
            const int cardId, const Card &cardData,
            const NodeRectData &nodeRectData, const bool saveCreatedNodeRectData);

    //!
    //! Do not call this method while iterating through `cardIdToNodeRect`.
    //! Does not check NodeRect::canClose().
    //!
    void closeNodeRect(const int cardId, const bool removeConnectedEdgeArrows);

    // edge arrow

    //!
    //! NodeRect's for start/end cards must exist.
    //! The returned EdgeArrow is already added to the scene.
    //!
    EdgeArrow *createEdgeArrow(
            const RelationshipId relId, const EdgeArrowData &edgeArrowData);

    void updateEdgeArrow(const RelationshipId relId);

    //!
    //! Do not call this method while iterating through `relIdToEdgeArrow`.
    //!
    void removeEdgeArrows(const QSet<RelationshipId> &relIds);

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);
    void setViewTopLeftPos(const QPointF &scenePos);

    QSet<RelationshipId> getEdgeArrowsConnectingNodeRect(const int cardId);
    static QLineF computeEdgeArrowLine(const QRectF &startNodeRect, const QRectF &endNodeRect);
};

#endif // BOARDVIEW_H
