#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QKeySequence>
#include <QShortcut>
#include "main_window.h"
#include "widgets/board_view.h"
#include "ui_main_window.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setUpWidgets();
    setUpConnections();
    setKeyboardShortcuts();

    // [temp]
    boardView->loadBoard(0, [](bool ok) {
        qInfo().noquote() << QString("load board %1").arg(ok ? "successful" : "failed");
    });
    noBoardOpenSign->setVisible(false);
    boardView->setVisible(true);


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

void MainWindow::closeEvent(QCloseEvent *event) {
    event->accept();
}

void MainWindow::setUpWidgets() {
    this->setWindowTitle(qApp->applicationName());

    // prevent left sidebar from resizing when the window resizes
    ui->splitter->setStretchFactor(0, 0); // (index, factor)
    ui->splitter->setStretchFactor(1, 1);

    //
    ui->frameLeftSideBar->setMinimumWidth(leftSideBarWidthMin);

    // set up ui->frameCentralArea
    {
        auto *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        ui->frameCentralArea->setLayout(layout);

        boardView = new BoardView;
        layout->addWidget(boardView);
        boardView->setVisible(false);

        noBoardOpenSign = new QLabel("No board is open");
        layout->addWidget(noBoardOpenSign);
        layout->setAlignment(noBoardOpenSign, Qt::AlignCenter);
    }

    //
    noBoardOpenSign->setStyleSheet(
            "QLabel {"
            "  color: #808080;"
            "  font-size: 14pt;"
            "  font-weight: bold;"
            "}");
}

void MainWindow::setUpConnections() {
}

void MainWindow::setKeyboardShortcuts() {
    {
        auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
        connect(shortcut, &QShortcut::activated, [this]() { close(); });
    }
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
