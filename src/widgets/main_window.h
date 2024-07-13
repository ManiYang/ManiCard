#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QLabel>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class BoardView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    // component widgets
    BoardView *boardView {nullptr};
    QLabel *noBoardOpenSign {nullptr};

    // states & constants
    const int leftSideBarWidthMin {60};
    const int leftSideBarWidthDefault {150};

    bool isEverShown {false};

    // setup
    void setUpWidgets();
    void setUpConnections();
    void setKeyboardShortcuts();

    // event handlers
    void onShownForFirstTime();
};

#endif // MAIN_WINDOW_H
