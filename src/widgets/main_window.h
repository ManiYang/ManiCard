#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QLabel>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ActionDebouncer;
class BoardsList;
class BoardView;

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

private:
    Ui::MainWindow *ui;

    // component widgets
    BoardsList *boardsList {nullptr};
    BoardView *boardView {nullptr};
    QLabel *noBoardOpenSign {nullptr};

    // states & constants
    static constexpr int leftSideBarWidthMin {60};
    static constexpr int leftSideBarWidthDefault {150};

    bool isEverShown {false};

    enum ClosingState {NotClosing, Closing, CloseNow};
    ClosingState closingState {ClosingState::NotClosing};

    //
    ActionDebouncer *saveWindowSizeDebounced;

    //
    void setUpWidgets();
    void setUpConnections();
    void setKeyboardShortcuts();

    //
    void onShownForFirstTime();
    void startUp();
    void prepareToClose();

    void onBoardSelectedByUser(const int boardId);
    void onUserToCreateNewBoard();
    void onUserToRemoveBoard(const int boardId);

    void saveTopLeftPosOfCurrentBoard(std::function<void (bool ok)> callback);
    void saveBoardsOrdering(std::function<void (bool ok)> callback);
};

#endif // MAIN_WINDOW_H
