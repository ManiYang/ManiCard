#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>
#include "models/board.h"
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
    //! \param boardIdToLoad: if = -1, will only close the board
    //! \param callback: parameter \e highlightedCardIdChanged will be true if highlighted Card ID
    //!                  changed to -1
    //!
    void loadBoard(
            const int boardIdToLoad,
            std::function<void (bool loadOk, bool highlightedCardIdChanged)> callback);

    void prepareToClose();

    void showButtonRightSidebar();

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
    QString boardName;
    QVector<Board::LabelAndColor> cardLabelsAndAssociatedColors;
            // in the order of precedence (high to low)
    QColor defaultNodeRectColor;

    //
    BoardViewToolBar *toolBar {nullptr};
    QGraphicsView *graphicsView {nullptr};
    GraphicsScene *graphicsScene {nullptr};
    QGraphicsRectItem *canvas {nullptr}; // draw everything on this

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

    // event handlers
    void onUserToOpenExistingCard(const QPointF &scenePos);
    void onUserToCreateNewCard(const QPointF &scenePos);
    void onUserToSetLabels(const int cardId);
    void onUserToCreateRelationship(const int cardId);
    void onUserToCloseNodeRect(const int cardId);
    void onUserToSetCardColors();
    void onBackgroundClicked();
    void onUserToZoomInOut(const bool zoomIn);

    //

    //!
    //! Remove all NodeRect's and EdgeArrow's. Does not check canClose().
    //!
    void closeAllCards(bool *highlightedCardIdChanged);

    //!
    //! Call this whenever graphicsView is resized, or graphics items are added/moved/removed.
    //! However, this can be slow for large graphics scene.
    //!
    void adjustSceneRect();

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

        //!
        //! \param highlightedCardIdChanged: will be true if highlighted Card ID changes to -1
        //!
        void unhighlightAllCards(bool *highlightedCardIdChanged);

        void updateAllNodeRectColors();

        bool contains(const int cardId) const;
        NodeRect *get(const int cardId) const;
        QSet<int> getAllCardIds() const;
        QColor getNodeRectOwnColor(const int cardId) const;
        QRectF getBoundingRectOfAllNodeRects() const; // returns QRectF() if no NodeRect exists

    private:
        BoardView *boardView;
        QHash<int, NodeRect *> cardIdToNodeRect;
        QHash<int, QColor> cardIdToNodeRectOwnColor;
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

    static QColor computeNodeRectDisplayColor(
            const QColor &nodeRectOwnColor,
            const QSet<QString> &cardLabels,
            const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &boardDefaultColor);

    QSet<RelationshipId> getEdgeArrowsConnectingNodeRect(const int cardId);
};

#endif // BOARDVIEW_H
