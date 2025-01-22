#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QAbstractButton>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QScreen>
#include <QToolButton>
#include "utilities/periodic_checker.h"
#include "widgets/icons.h"

namespace Ui {
class MainWindow;
}

class ActionDebouncer;
class RightSidebar;
class WorkspaceFrame;
class WorkspacesList;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //!
    //! Before calling this method:
    //!   + \c this must be visible
    //!   + \c canReload() must return true
    //!
    void load(std::function<void (bool ok)> callback);

    void prepareToReload();
    bool canReload();

signals:
    void userToReloadApp();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    Ui::MainWindow *ui;

    // component widgets
    WorkspacesList *workspacesList {nullptr};
    WorkspaceFrame *workspaceFrame {nullptr};

    QLabel *noWorkspaceOpenSign {nullptr};
    QToolButton *buttonOpenMainMenu {nullptr};

    RightSidebar *rightSidebar {nullptr};

    // menus and actions
    QMenu *mainMenu;
    QAction *actionQuit {nullptr};

    // states & constants
    static constexpr int leftSideBarWidthMin {60};
    static constexpr int leftSideBarWidthDefault {150};
    static constexpr int rightSideBarWidthDefault {200};

    bool isEverShown {false};

    enum ClosingState {NotClosing, Closing, CloseNow};
    ClosingState closingState {ClosingState::NotClosing};

    const QScreen *currentScreen {nullptr}; // used only in checkIsScreenChanged()

    QHash<QAbstractButton *, Icon> buttonToIcon; // set in setUpWidgets()

    //
    void setUpWidgets();
    void setUpConnections();
    void setUpButtonsWithIcons();
    void setUpMainMenu();

    // ==== event handlers ====

    void onShownForFirstTime();

    void onWorkspaceSelectedByUser(const int workspaceId);
    void onUserToCreateNewWorkspace();
    void onUserRenamedWorkspace(const int workspaceId, const QString &newName);
    void onUserToRemoveWorkspace(const int workspaceId);

    void onUserToSetCardLabelsList();
    void onUserToSetRelationshipTypesList();

    void onUserCloseWindow();
    void onUserToReload();
    void openOptionsDialog();

    // -- event handling tools
    ActionDebouncer *saveWindowSizePosDebounced;

    void saveBeforeClose();

    void saveTopLeftPosAndZoomRatioOfCurrentBoard();

    //!
    //! save the opened board in `workspaceFrame`
    //!
    void saveLastOpenedBoardOfCurrentWorkspace();

    //!
    //! save the ordering of workspaces in `workspacesList`
    //!
    void saveWorkspacesOrdering();

    void saveLastOpenedWorkspace();

    void checkIsScreenChanged();
};

#endif // MAIN_WINDOW_H
