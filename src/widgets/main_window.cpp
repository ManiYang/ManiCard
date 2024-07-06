#include <QApplication>
#include <QDebug>
#include "main_window.h"
#include "widgets/board_view.h"
#include "ui_main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(qApp->applicationName());

    setUpWidgets();
    setUpConnections();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showEvent(QShowEvent */*event*/) {
    if (!isEverShown){
        isEverShown = true;
        onShownForFirstTime();
    }
}

void MainWindow::setUpWidgets() {
    ui->frameLeftSideBar->setMinimumWidth(leftSideBarWidthMin);

    // prevent left sidebar from resizing when the window resizes
    ui->splitter->setStretchFactor(0, 0); // (index, factor)
    ui->splitter->setStretchFactor(1, 1);

    // set up ui->frameCentralArea
    {
        auto *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        ui->frameCentralArea->setLayout(layout);

        boardView = new BoardView;
        layout->addWidget(boardView);
    }
}

void MainWindow::setUpConnections() {
}

void MainWindow::onShownForFirstTime() {
    {
        // adjust the size of left sidebar
        const int leftSideBarSizeTarget = leftSideBarWidthDefault;

        const QList<int> sizes = ui->splitter->sizes(); // [0]: left sidebar, [1]: central area
        if (sizes.count() == 2) {
            const int sum = sizes.at(0) + sizes.at(1);
            if (sum >= leftSideBarSizeTarget * 2)
                ui->splitter->setSizes({leftSideBarSizeTarget, sum - leftSideBarSizeTarget});
        }
        else {
            Q_ASSERT(false);
        }
    }
}
