#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>

class Card;
class CardPropertiesUpdate;
class GraphicsScene;
class NodeRect;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    bool canClose() const;

    //!
    //! \param boardId_: if = -1, will only close the board
    //! \param callback
    //!
    void loadBoard(const int boardIdToLoad, std::function<void (bool ok)> callback);

    int getBoardId() const; // can be -1

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    const QSizeF defaultNewNodeRectSize {200, 120};

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
    void openExistingCard(const int cardId, const QPointF &scenePos);
    void userToCreateNewCard(const QPointF &scenePos);
    void saveCardPropertiesUpdate(
            NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
            std::function<void ()> callback); // callback will be called in context of `this`
    void closeAllCards(); // does not check canClose()

    // node rect creation and removal

    //!
    //! Returned NodeRect is already added to the scene.
    //!
    NodeRect *createNodeRect(const int cardId, const Card &cardData);

    void closeNodeRect(const int cardId); // does not check NodeRect::canClose()

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);
    void setViewTopLeftPos(const QPointF &scenePos);
};

#endif // BOARDVIEW_H
