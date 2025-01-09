#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QScreen>
#include <QToolButton>
#include "utilities/periodic_checker.h"

namespace Ui {
class MainWindow;
}

class ActionDebouncer;
class BoardsList;
class BoardView;
class RightSidebar;
class WorkspacesList;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    Ui::MainWindow *ui;

    // component widgets
//    BoardsList *boardsList {nullptr};
    WorkspacesList *workspacesList {nullptr};
    BoardView *boardView {nullptr};

    QLabel *noBoardOpenSign {nullptr};
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

    //
    void setUpWidgets();
    void setUpConnections();
    void setUpMainMenu();

    // ==== event handlers ====

    void onShownForFirstTime();
    void onStartUp();

    void onBoardSelectedByUser(const int boardId);
    void onUserToCreateNewBoard();
    void onUserToRemoveBoard(const int boardId);

    void onUserToSetCardLabelsList();
    void onUserToSetRelationshipTypesList();

    void onUserCloseWindow();

    // -- event handling tools
    ActionDebouncer *saveWindowSizeDebounced;

    void saveTopLeftPosAndZoomRatioOfCurrentBoard();
    void saveBoardsOrdering();

    void checkIsScreenChanged();
};

#endif // MAIN_WINDOW_H
