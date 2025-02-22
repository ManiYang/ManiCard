#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QAbstractButton>
#include <QButtonGroup>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QScreen>
#include <QStackedWidget>
#include <QToolButton>
#include "utilities/periodic_checker.h"
#include "widgets/icons.h"

namespace Ui {
class MainWindow;
}

class ActionDebouncer;
class RightSidebar;
class SearchPage;
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
    WorkspaceFrame *workspaceFrame {nullptr};
    QLabel *noWorkspaceOpenSign {nullptr};

    // -- left sidebar
    struct LeftSidebar
    {
        explicit LeftSidebar(MainWindow *mainWindow);
        void addStackedWidgetToLayout(QLayout *layout);
        void addPage(QWidget *widget, QToolButton *button);
                // QToolButton::clicked() signal will be connected
        void setCurrentPage(QWidget *widget);
    private:
        MainWindow *mainWindow;
        QStackedWidget *stackedWidget;
        QButtonGroup *buttonGroup;
        QHash<QWidget *, QToolButton *> pageToButton;
    };
    LeftSidebar leftSidebar {this};

    QToolButton *buttonOpenMainMenu {nullptr};

    WorkspacesList *workspacesList {nullptr};
    SearchPage *searchPage {nullptr};

    // -- right side bar
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

    //!
    //! \param workspaceId
    //! \param boardId: the board to open (if found in \e workspaceId). If is -1, will open
    //!                 the last-opened board.
    //! \param cardId: the opened card (NodeRect) to highlight and to center on (if found
    //!                in \e boardId)
    //!
    void onUserToOpenWorkspace(
            const int workspaceId, const int boardId = -1, const int cardId = -1);

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
