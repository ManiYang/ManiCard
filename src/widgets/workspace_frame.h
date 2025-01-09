#ifndef WORKSPACEFRAME_H
#define WORKSPACEFRAME_H

#include <QFrame>
#include <QLabel>
#include <QTabBar>
#include <QToolButton>
#include "widgets/components/simple_toolbar.h"

class BoardView;
class WorkspaceToolBar;

class WorkspaceFrame : public QFrame
{
    Q_OBJECT
public:
    explicit WorkspaceFrame(QWidget *parent = nullptr);

    //!
    //! \brief loadWorkspace
    //! \param workspaceId: if is -1, will only close the workspace
    //! \param callback
    //!
    void loadWorkspace(
            const int workspaceId,
            std::function<void (bool ok, bool highlightedCardIdChanged)> callback);

    void showButtonRightSidebar();

    void prepareToClose();

    //
    bool canClose() const;

signals:
    void openRightSidebar();

private:
    int workspaceId {-1};
    QString workspaceName;

    WorkspaceToolBar *workspaceToolBar {nullptr};
    QTabBar *boardsTabBar {nullptr};
    BoardView *boardView {nullptr};
    QLabel *noBoardSign {nullptr};
    QMenu *boardTabContextMenu {nullptr};

    //
    void setUpWidgets();
    void setUpConnections();
    void setUpBoardTabContextMenu();

    //
    void onUserToAddBoard();
    void onUserToRenameBoard(const int boardId, const QString &originalName);
};

//========

class WorkspaceToolBar : public SimpleToolBar
{
    Q_OBJECT
public:
    explicit WorkspaceToolBar(QWidget *parent = nullptr);

    void setWorkspaceName(const QString &name);
    void showButtonOpenRightSidebar();

signals:
    void openRightSidebar();
    void openCardColorsDialog();

private:
    QLabel *labelWorkspaceName {nullptr};
    QToolButton *buttonOpenRightSidebar {nullptr};
    QToolButton *buttonWorkspaceSettings {nullptr};
    QMenu *workspaceSettingsMenu {nullptr};

    void setUpWorkspaceSettingsMenu();
    void setUpConnections();
};

#endif // WORKSPACEFRAME_H
