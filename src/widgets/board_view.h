#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>
#include "models/board.h"
#include "models/custom_data_query.h"
#include "models/edge_arrow_data.h"
#include "models/node_rect_data.h"
#include "models/relationship.h"

class BoardViewToolBar;
class DataViewBox;
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
    enum class ZoomAction { ZoomIn, ZoomOut, ResetZoom };
    void applyZoomAction(const ZoomAction zoomAction);

    //
    int getBoardId() const; // can be -1
    QPointF getViewTopLeftPos() const; // in canvas coordinates
    double getZoomRatio() const;

    bool canClose() const;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void openRightSideBar();

private:
    static inline const QSizeF defaultNewNodeRectSize {200, 120};
    static inline const QSizeF defaultNewDataViewBoxSize {400, 400};
    static inline const QColor defaultNewDataViewBoxColor {170, 170, 170};
    static inline const QColor defaultEdgeArrowLineColor {100, 100, 100};
    constexpr static double defaultEdgeArrowLineWidth {2.0};

    constexpr static double zValueForNodeRects {10.0};
    constexpr static double zValueForEdgeArrows {15.0};

    int boardId {-1}; // -1: no board loaded
    QString boardName;
    QVector<Board::LabelAndColor> cardLabelsAndAssociatedColors;
            // in the order of precedence (high to low)
    QColor defaultNodeRectColor;

    double zoomScale {1.0};
    double graphicsGeometryScaleFactor {1.0};

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
    void onUserToDuplicateCard(const QPointF &scenePos);
    void onUserToCreateNewCustomDataQuery(const QPointF &scenePos);
    void onUserToSetLabels(const int cardId);
    void onUserToCreateRelationship(const int cardId);
    void onUserToCloseNodeRect(const int cardId);
    void onUserToCloseDataViewBox(const int customDataQueryId);
    void onUserToSetCardColors();
    void onBackgroundClicked();

    //

    //!
    //! Remove all NodeRect's, EdgeArrow's, & DataViewBox's. Does not check canClose().
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

        //!
        //! \param highlightedCardIdChanged: will be true if highlighted Card ID changes to -1
        //!
        void unhighlightAllCards(bool *highlightedCardIdChanged);

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
    EdgeArrowsCollection edgeArrowsCollection {this};

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

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos) const;
    QPointF getViewCenterInScene() const;
    void setViewTopLeftPos(const QPointF &canvasPos);
    void moveSceneRelativeToView(const QPointF &displacement); // displacement: in pixel

    static QColor computeNodeRectDisplayColor(
            const QColor &nodeRectOwnColor,
            const QSet<QString> &cardLabels,
            const QVector<Board::LabelAndColor> &cardLabelsAndAssociatedColors,
            const QColor &boardDefaultColorForNodeRect);
    static QColor computeDataViewBoxDisplayColor(
            const QColor &dataViewBoxOwnColor, const QColor &boardDefaultColorForDataViewBox);

    QSet<RelationshipId> getEdgeArrowsConnectingNodeRect(const int cardId);
};

#endif // BOARDVIEW_H
