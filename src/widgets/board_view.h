#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>

class Card;
class CardPropertiesUpdate;
class GraphicsScene;
class NodeRect;
struct NodeRectData;

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
    const QSizeF defaultNewNodeRectSize {200, 120};
    const QColor defaultNewNodeRectColor {170, 170, 170};

    int boardId {-1}; // -1: no board loaded
    QHash<int, NodeRect *> cardIdToNodeRect;
            // currently opened cards. Updated in createNodeRect() & closeNodeRect().

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
    void userToCloseNodeRect(const int cardId);
    void openExistingCard(const int cardId, const QPointF &scenePos);
    void saveCardPropertiesUpdate(
            NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
            std::function<void ()> callback); // callback will be called in context of `this`
    void closeAllCards(); // does not check canClose()

    // node rect creation and removal

    //!
    //! Returned NodeRect is already added to the scene.
    //!
    NodeRect *createNodeRect(
            const int cardId, const Card &cardData,
            const NodeRectData &nodeRectData, const bool saveCreatedNodeRectData);

    void closeNodeRect(const int cardId); // does not check NodeRect::canClose()

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);
    void setViewTopLeftPos(const QPointF &scenePos);
};

#endif // BOARDVIEW_H
