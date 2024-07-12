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

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    const QSizeF defaultNewNodeRectSize {200, 120};

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

    //
    bool isEverShown {false};

    // setup
    void setUpWidgets();
    void setUpContextMenu();
    void setUpConnections();
    void installEventFiltersOnComponents();
    QString styleSheet();

    //
    void onShownForFirstTime();
    void onGraphicsViewResize();

    void userToOpenExistingCard(const QPointF &scenePos);
    void openExistingCard(const int cardId, const QPointF &scenePos);
    void userToCreateNewCard(const QPointF &scenePos);
    void saveCardPropertiesUpdate(
            NodeRect *nodeRect, const CardPropertiesUpdate &propertiesUpdate,
            std::function<void ()> callback); // callback will be called in context of `this`

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);

    //!
    //! Returned NodeRect is added to the scene.
    //!
    NodeRect *createNodeRect(const int cardId, const Card &cardData);
};

#endif // BOARDVIEW_H
