#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

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

private:
    Ui::MainWindow *ui;
    void setUpWidgets();
    void setUpConnections();

    // component widgets
    BoardView *boardView {nullptr};

    // states & constants
    const int leftSideBarWidthMin {60};
    const int leftSideBarWidthDefault {150};

    bool isEverShown {false};

    // event handlers
    void onShownForFirstTime();

};

#endif // MAIN_WINDOW_H
