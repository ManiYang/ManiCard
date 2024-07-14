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
    switch (closingState) {
    case MainWindow::NotClosing:
        event->ignore();
        closingState = ClosingState::Closing;

        boardsList->setEnabled(false);
        boardView->setEnabled(false);

        saveDataOnClose();

        return;

    case MainWindow::Closing:
        event->ignore();
        return;

    case MainWindow::CloseNow:
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
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
            this, [this](int newBoardId, int /*previousBoardId*/) {
        onBoardSelectedByUser(newBoardId);
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

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        BoardsListProperties boardsListProperties;
        QHash<int, QString> boardsIdToName;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    boardsList->setEnabled(false);
    boardView->setEnabled(false);

    //
    routine->addStep([this, routine]() {
        // get boards-list properties
        Services::instance()->getCachedDataAccess()->getBoardsListProperties(
                [routine](bool ok, BoardsListProperties properties) {
                    auto context = routine->continuationContext();

                    if (!ok) {
                        routine->errorMsg
                                = "Could not get boards list properties. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        routine->boardsListProperties = properties;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // get all board IDs
        Services::instance()->getCachedDataAccess()->getBoardIdsAndNames(
                [routine](bool ok, const QHash<int, QString> &idToName) {
                    auto context = routine->continuationContext();

                    if (!ok) {
                        routine->errorMsg
                                = "Could not get the list of boards. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        routine->boardsIdToName = idToName;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        auto context = routine->continuationContext();

        // populate `boardsList`
        boardsList->resetBoards(
                routine->boardsIdToName, routine->boardsListProperties.boardsOrdering);

        //
        noBoardOpenSign->setVisible(false);
        boardView->setVisible(true);

        // load last-opened board
        const int lastOpenedBoardId = routine->boardsListProperties.lastOpenedBoard;
        if (routine->boardsIdToName.contains(lastOpenedBoardId)) {
            boardView->loadBoard(
                    lastOpenedBoardId,
                    [this, lastOpenedBoardId](bool ok) {
                        if (ok)
                            boardsList->setSelectedBoardId(lastOpenedBoardId);
                    }
            );
        }
    }, this);

    routine->addStep([this, routine]() {
        // (final step)
        auto context = routine->continuationContext();

        boardsList->setEnabled(true);
        boardView->setEnabled(true);

        if (routine->errorFlag)
            createWarningMessageBox(this, " ", routine->errorMsg)->exec();
    }, this);

    routine->start();
}

void MainWindow::saveDataOnClose() {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool hasUnsavedUpdate {false};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. save last opened board & boards ordering
        BoardsListPropertiesUpdate propertiesUpdate;
        propertiesUpdate.lastOpenedBoard = boardsList->selectedBoardId();
        propertiesUpdate.boardsOrdering = boardsList->getBoardsOrder();

        Services::instance()->getCachedDataAccess()->updateBoardsListProperties(
                propertiesUpdate,
                // callback
                [this, routine](bool ok) {
                    auto context = routine->continuationContext();

                    if (!ok) {
                        const auto msg
                                = QString("Could not save boards-list properties to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        createWarningMessageBox(this, " ", msg)->exec();

                        routine->hasUnsavedUpdate = true;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 2. save current board's topLeftPos
        saveTopLeftPosOfCurrentBoard(
                // callback
                [routine](bool ok) {
                    auto context = routine->continuationContext();
                    if (!ok)
                        routine->hasUnsavedUpdate = true;
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        // 3.
        auto context = routine->continuationContext();

        if (routine->hasUnsavedUpdate) {
            const auto r
                    = QMessageBox::question(this, " ", "There is unsaved update. Exit anyway?");
            if (r != QMessageBox::Yes) {
                boardsList->setEnabled(true);
                boardView->setEnabled(true);
                closingState = ClosingState::NotClosing;
                return;
            }
        }

        closingState = ClosingState::CloseNow;
        close();
    }, this);

    routine->start();
}

void MainWindow::onBoardSelectedByUser(const int boardId) {
    auto *routine = new AsyncRoutine;

    // boardView->canClose() ...


    routine->addStep([this, routine]() {
        // save current board's topLeftPos
        saveTopLeftPosOfCurrentBoard(
                // callback
                [routine](bool /*ok*/) {
                    routine->nextStep();
                }
        );
    }, this);

    routine->addStep([this, routine, boardId]() {
        // load `boardId`
        noBoardOpenSign->setVisible(false);
        boardView->setVisible(true);

        boardView->loadBoard(
                boardId,
                [routine](bool /*ok*/) {
                    routine->nextStep();
                });
    }, this);

    routine->start();
}

void MainWindow::saveTopLeftPosOfCurrentBoard(std::function<void (bool)> callback) {
    const int currentBoardId = boardView->getBoardId();
    if (currentBoardId == -1) {
        callback(true);
        return;
    }

    BoardNodePropertiesUpdate propertiesUpdate;
    propertiesUpdate.topLeftPos = boardView->getViewTopLeftPos();

    Services::instance()->getCachedDataAccess()->updateBoardNodeProperties(
            currentBoardId, propertiesUpdate,
            // callback
            [this, callback](bool ok) {
                if (!ok) {
                    const auto msg
                            = QString("Could not save board's top-left position to DB.\n\n"
                                      "There is unsaved update. See %1")
                              .arg(Services::instance()->getUnsavedUpdateFilePath());
                    createWarningMessageBox(this, " ", msg)->exec();
                }
                callback(ok);
            },
            this
    );
}
