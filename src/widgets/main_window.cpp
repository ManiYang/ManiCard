#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include "cached_data_access.h"
#include "main_window.h"
#include "services.h"
#include "ui_main_window.h"
#include "utilities/async_routine.h"
#include "utilities/message_box.h"
#include "widgets/board_view.h"
#include "widgets/boards_list.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setUpWidgets();
    setUpConnections();
    setKeyboardShortcuts();

    startUp();
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

    // set up ui->frameLeftSideBar
    {
        boardsList = new BoardsList;
        ui->frameLeftSideBar->layout()->addWidget(boardsList);
    }

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
    connect(boardsList, &BoardsList::boardSelected,
            this, [this](int newBoardId, int previousBoardId) {
        // boardView->canClose() ...

        noBoardOpenSign->setVisible(false);
        boardView->setVisible(true);

        boardView->loadBoard(newBoardId, [](bool ok) {
            qInfo().noquote() << QString("load board %1").arg(ok ? "successful" : "failed");
        });
    });

    connect(boardsList, &BoardsList::userRenamedBoard,
            this, [this](int boardId, QString name) {
        BoardNodePropertiesUpdate update;
        update.name = name;

        Services::instance()->getCachedDataAccess()->updateBoardNodeProperties(
                boardId, update,
                // callback
                [this](bool ok) {
                    if (!ok) {
                        const auto msg
                                = QString("Could not save board name to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();
                    }
                },
                this
        );
    });

//    boardsOrderChanged(QVector<int> boardIds);
//    userToCreateNewBoard();
//    userToRemoveBoard(int boardId);




}

void MainWindow::setKeyboardShortcuts() {
    {
        auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
        connect(shortcut, &QShortcut::activated, this, [this]() { close(); });
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

void MainWindow::startUp() {
//    // [temp]
//    boardView->loadBoard(0, [](bool ok) {
//        qInfo().noquote() << QString("load board %1").arg(ok ? "successful" : "failed");
//    });
//    noBoardOpenSign->setVisible(false);
//    boardView->setVisible(true);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QVector<int> boardsOrdering;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine]() {
        // get boards ordering
        Services::instance()->getCachedDataAccess()->getBoardsOrdering(
                [routine](bool ok, const QVector<int> &ordering) {
                    auto context = routine->continuationContext();

                    if (!ok) {
                        routine->errorMsg = "Could not get boards data. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        routine->boardsOrdering = ordering;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // get board IDs
        Services::instance()->getCachedDataAccess()->getBoardIdsAndNames(
                [this, routine](bool ok, const QHash<int, QString> &idToName) {
                    auto context = routine->continuationContext();

                    if (!ok) {
                        routine->errorMsg = "Could not get list of boards. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        // populate `boardsList`
                        boardsList->resetBoards(idToName, routine->boardsOrdering);
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // (final step)
        auto context = routine->continuationContext();
        if (routine->errorFlag)
            createWarningMessageBox(this, " ", routine->errorMsg)->exec();
    }, this);

    routine->start();
}
