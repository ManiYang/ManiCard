#ifndef WORKSPACEFRAME_H
#define WORKSPACEFRAME_H

#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include "widgets/common_types.h"
#include "widgets/components/simple_toolbar.h"
#include "widgets/icons.h"

class BoardView;
class CustomTabBar;
class NoBoardSign;
class WorkspaceToolBar;

class WorkspaceFrame : public QFrame
{
    Q_OBJECT
public:
    explicit WorkspaceFrame(QWidget *parent = nullptr);

    //!
    //! Before calling this method:
    //!   + \c this must be visible
    //!   + \c canClose() must returns true
    //! \param workspaceId: if is -1, will only close the workspace
    //! \param callback
    //!
    void loadWorkspace(
            const int workspaceId,
            std::function<void (bool ok, bool highlightedCardIdChanged)> callback);

    void changeWorkspaceName(const QString newName);
    void showButtonRightSidebar();
    void applyZoomAction(const ZoomAction zoomAction);
    void toggleCardPreview();

    void prepareToClose();

    //

    int getWorkspaceId() const;

    //!
    //! \return -1 if no board is open
    //!
    int getCurrentBoardId();

    QSet<int> getAllBoardIds() const;

    QPointF getBoardViewTopLeftPos() const; // in canvas coordinates
    double getBoardViewZoomRatio() const;

    bool canClose() const;

signals:
    void openRightSidebar();

private:
    int workspaceId {-1};
    QString workspaceName;

    WorkspaceToolBar *workspaceToolBar {nullptr};
    CustomTabBar *boardsTabBar {nullptr};
    BoardView *boardView {nullptr};
    NoBoardSign *noBoardSign {nullptr};

    struct ContextMenu
    {
        explicit ContextMenu(WorkspaceFrame *workspaceFrame);
        QMenu *menu;
        int boardTabContextMenuTargetBoardId {-1};

        void setActionIcons();
    private:
        WorkspaceFrame *workspaceFrame;
        QHash<QAction *, Icon> actionToIcon;
    };
    ContextMenu boardTabContextMenu {this};

    //
    void setUpWidgets();
    void setUpConnections();

    //
    void onUserToAddBoard();
    void onUserToRenameBoard(const int boardId);
    void onUserSelectedBoard(const int boardId);
    void onUserToRemoveBoard(const int boardIdToRemove);
    void onUserToSetCardColors();

    //!
    //! save the data for the board currently shown in `boardView`
    //!
    void saveTopLeftPosAndZoomRatioOfCurrentBoard();

    //!
    //! save the ordering of boards in `boardsTabBar`
    //!
    void saveBoardsOrdering();
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
    void userToAddNewBoard();
    void openRightSidebar();
    void openCardColorsDialog();

private:
    QLabel *labelWorkspaceName {nullptr};
    QToolButton *buttonOpenRightSidebar {nullptr};
    QToolButton *buttonWorkspaceSettings {nullptr};
    QMenu *workspaceSettingsMenu {nullptr};

    QHash<QAbstractButton *, Icon> buttonToIcon;

    void setUpWorkspaceSettingsMenu();
    void setUpConnections();
    void setUpButtonsWithIcons();
};

//========

class NoBoardSign : public QFrame
{
    Q_OBJECT
public:
    explicit NoBoardSign(QWidget *parent = nullptr);

signals:
    void userToAddBoard();
};

#endif // WORKSPACEFRAME_H
