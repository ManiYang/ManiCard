#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>
#include <QGraphicsView>
#include <QMenu>

class GraphicsScene;

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;

private:
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

    // event handlers
    void onShownForFirstTime();
    void onGraphicsViewResize();

    void userToOpenExistingCard(const QPointF &scenePos);
    void openExistingCard(const int cardId, const QPointF &scenePos);

    // tools
    QPoint getScreenPosFromScenePos(const QPointF &scenePos);
};

#endif // BOARDVIEW_H
