#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QIcon>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include "cached_data_access.h"
#include "main_window.h"
#include "services.h"
#include "ui_main_window.h"
#include "utilities/action_debouncer.h"
#include "utilities/async_routine.h"
#include "utilities/margins_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "widgets/board_view.h"
#include "widgets/boards_list.h"
#include "widgets/dialogs/dialog_user_relationship_types.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow)
        , mainMenu(new QMenu(this)) {
    ui->setupUi(this);
    setUpWidgets();
    setUpConnections();
    setUpActions();
    setUpMainMenu();

    //
    saveWindowSizeDebounced = new ActionDebouncer(
            1000, ActionDebouncer::Option::Delay,
            [this]() {
                Services::instance()->getCachedDataAccess()->saveMainWindowSize(this->size());
            },
            this
    );

    //
    startUp();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event) {
    if (!isEverShown){
        isEverShown = true;
        onShownForFirstTime();
    }
    QMainWindow::showEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    if (event->spontaneous())
        saveWindowSizeDebounced->tryAct();

    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    switch (closingState) {
    case MainWindow::NotClosing:
        event->ignore();
        closingState = ClosingState::Closing;

        boardsList->setEnabled(false);
        boardView->setEnabled(false);

        prepareToClose();

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
        auto *leftSideBarLayout = dynamic_cast<QBoxLayout *>(ui->frameLeftSideBar->layout());
        Q_ASSERT(leftSideBarLayout != nullptr);

        // tool bar
        auto *hLayout = new QHBoxLayout;
        leftSideBarLayout->addLayout(hLayout);
        hLayout->setContentsMargins(0, 0, 0, 0);
        {
            buttonOpenMainMenu = new QToolButton;
            buttonOpenMainMenu->setIcon(QIcon(":/icons/menu4_black_24"));
            buttonOpenMainMenu->setIconSize({24, 24});
            hLayout->addWidget(buttonOpenMainMenu);

            //
            hLayout->addStretch();
        }

        // boards list
        boardsList = new BoardsList;
        leftSideBarLayout->addWidget(boardsList);
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
    buttonOpenMainMenu->setStyleSheet(
            "QToolButton {"
            "  border: none;"
            "  background: transparent;"
            "}"
            "QToolButton:hover {"
            "  background: #e0e0e0;"
            "}");
    noBoardOpenSign->setStyleSheet(
            "QLabel {"
            "  color: #808080;"
            "  font-size: 14pt;"
            "  font-weight: bold;"
            "}");
}

void MainWindow::setUpConnections() {
    // buttonOpenMainMenu
    connect(buttonOpenMainMenu, &QToolButton::clicked, this, [this]() {
        const auto w = buttonOpenMainMenu->width();
        mainMenu->popup(buttonOpenMainMenu->mapToGlobal({w, 0}));
    });

    // mainMenu
    connect(mainMenu, &QMenu::aboutToHide, this, [this]() {
        buttonOpenMainMenu->update(); // without this, the button's appearence stay in hover state
    });

    // boardsList
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
                        showWarningMessageBox(this, " ", msg);
                    }
                },
                this
        );
    });

    connect(boardsList, &BoardsList::userToCreateNewBoard, this, [this]() {
        onUserToCreateNewBoard();
    });

    connect(boardsList, &BoardsList::userToRemoveBoard, this, [this](int boardId) {
        const auto r = QMessageBox::question(
                this, " ",
                QString("Delete the board \"%1\"?").arg(boardsList->boardName(boardId)));
        if (r != QMessageBox::Yes)
            return;

        onUserToRemoveBoard(boardId);
    });

    connect(boardsList, &BoardsList::boardsOrderChanged, this, [this](QVector<int> /*boardIds*/) {
        saveBoardsOrdering([this](bool ok) {
            if (!ok) {
                const auto msg
                        = QString("Could not save boards ordering to DB.\n\n"
                                  "There is unsaved update. See %1")
                          .arg(Services::instance()->getUnsavedUpdateFilePath());
                showWarningMessageBox(this, " ", msg);
            }
        });
    });
}

void MainWindow::setUpActions() {
    actionQuit = new QAction("Quit", this);
    this->addAction(actionQuit);
    actionQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(actionQuit, &QAction::triggered, this, [this]() {
        close();
    });
}

void MainWindow::setUpMainMenu() {
    {
        auto *menu = mainMenu->addMenu("Graph");
        {
            menu->addAction(
                    "Labels...",
                    this, [this]() {
                        // show labels list dialog ...............

                    }
            );
            menu->addAction(
                    "Relationship Types...",
                    this, [this]() { showRelationshipTypesDialog(); }
            );
        }
    }
    mainMenu->addSeparator();
    mainMenu->addAction(actionQuit);
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
    {
        const auto sizeOpt
                = Services::instance()->getCachedDataAccess()->getMainWindowSize();
        if (sizeOpt.has_value())
            resize(sizeOpt.value());
    }

    boardsList->setEnabled(false);
    boardView->setEnabled(false);

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        BoardsListProperties boardsListProperties;
        QHash<int, QString> boardsIdToName;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get boards-list properties
        Services::instance()->getCachedDataAccess()->getBoardsListProperties(
                [routine](bool ok, BoardsListProperties properties) {
                    ContinuationContext context(routine);

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
                    ContinuationContext context(routine);

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
                    // callback
                    [this, routine, lastOpenedBoardId](bool ok) {
                        ContinuationContext context(routine);
                        if (ok) {
                            boardsList->setSelectedBoardId(lastOpenedBoardId);
                        }
                        else {
                            routine->errorMsg
                                    = QString("Could not load board %1").arg(lastOpenedBoardId);
                            context.setErrorFlag();
                        }
                    }
            );
        }
        else {
            routine->nextStep();
        }
    }, this);

    routine->addStep([this, routine]() {
        // (final step)
        ContinuationContext context(routine);

        boardsList->setEnabled(true);
        boardView->setEnabled(true);

        if (routine->errorFlag)
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::prepareToClose() {
    saveWindowSizeDebounced->actNow();

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool hasUnsavedUpdate {false};
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([routine]() {
        // wait until CachedDataAccess::hasWriteRequestInProgress() returns false
        (new PeriodicChecker)->setPeriod(20)->setTimeOut(6000)
                ->setPredicate([]() {
                    return !Services::instance()
                            ->getCachedDataAccess()->hasWriteRequestInProgress();
                })
                ->onPredicateReturnsTrue([routine]() {
                    routine->nextStep();
                })
                ->onTimeOut([routine]() {
                    ContinuationContext context(routine);
                    context.setErrorFlag();
                    qWarning().noquote()
                            << "Time-out while awaiting all saving operations to finish";
                })
                ->setAutoDelete()->start();
    }, this);

    routine->addStep([this, routine]() {
        // save last opened board
        BoardsListPropertiesUpdate propertiesUpdate;
        propertiesUpdate.lastOpenedBoard = boardsList->selectedBoardId();

        Services::instance()->getCachedDataAccess()->updateBoardsListProperties(
                propertiesUpdate,
                // callback
                [this, routine](bool ok) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        const auto msg
                                = QString("Could not save last-opened board to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                        showWarningMessageBox(this, " ", msg);

                        routine->hasUnsavedUpdate = true;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // save current board's topLeftPos
        saveTopLeftPosOfCurrentBoard(
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        routine->hasUnsavedUpdate = true;
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        //
        ContinuationContext context(routine);

        if (routine->hasUnsavedUpdate) {
            const auto r
                    = QMessageBox::question(this, " ", "There is unsaved update. Exit anyway?");
            if (r != QMessageBox::Yes) {
                context.setErrorFlag();
                return;
            }
        }
    }, this);

    routine->addStep([this, routine]() {
        // (final step)
        ContinuationContext context(routine);

        if (routine->errorFlag) {
            boardsList->setEnabled(true);
            boardView->setEnabled(true);
            closingState = ClosingState::NotClosing;
        }
        else {
            closingState = ClosingState::CloseNow;
            close();
        }
    }, this);

    routine->start();
}

void MainWindow::onBoardSelectedByUser(const int boardId) {
    auto *routine = new AsyncRoutine;

    routine->addStep([this, routine]() {
        // save current board's topLeftPos
        saveTopLeftPosOfCurrentBoard(
                // callback
                [routine](bool /*ok*/) {
                    routine->nextStep();
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        boardView->prepareToClose();

        // wait until boardView->canClose() returns true
        (new PeriodicChecker)->setPeriod(50)->setTimeOut(20000)
            ->setPredicate([this]() {
                return boardView->canClose();
            })
            ->onPredicateReturnsTrue([routine]() {
                routine->nextStep();
            })
            ->onTimeOut([routine]() {
                qWarning().noquote() << "time-out while awaiting BoardView::canClose()";
                routine->nextStep();
            })
            ->setAutoDelete()->start();
    }, this);

    routine->addStep([this, routine, boardId]() {
        // load `boardId`
        noBoardOpenSign->setVisible(false);
        boardView->setVisible(true);

        boardView->loadBoard(
                boardId,
                [=](bool ok) {
                    if (!ok) {
                        QMessageBox::warning(
                                this, " ", QString("Could not load board %1").arg(boardId));
                    }
                    routine->nextStep();
                });
    }, this);

    routine->start();
}

void MainWindow::onUserToCreateNewBoard() {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newBoardId {-1};
        Board boardData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. get new board ID
        Services::instance()->getCachedDataAccess()->requestNewBoardId(
                //callback
                [routine](std::optional<int> boardId) {
                    ContinuationContext context(routine);

                    if (!boardId.has_value()) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get new board ID. See logs for details.";
                    }
                    else {
                        routine->newBoardId = boardId.value();
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 2. add to `boardsList`
        ContinuationContext context(routine);

        routine->boardData.name = "new board";
        routine->boardData.topLeftPos = QPointF(0, 0);

        boardsList->addBoard(routine->newBoardId, routine->boardData.name);
        boardsList->startEditBoardName(routine->newBoardId);
    }, this);

    routine->addStep([this, routine]() {
        // 3. save boards ordering
        saveBoardsOrdering([routine](bool ok) {
            ContinuationContext context(routine);
            if (!ok) {
                context.setErrorFlag();
                routine->errorMsg
                        = QString("Could not save boards ordering to DB.\n\n"
                                  "There is unsaved update. See %1")
                          .arg(Services::instance()->getUnsavedUpdateFilePath());
            }
        });
    }, this);

    routine->addStep([this, routine]() {
        // 3. create new board in DB
        Services::instance()->getCachedDataAccess()->createNewBoardWithId(
                routine->newBoardId, routine->boardData,
                //callback
                [routine](bool ok) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Could not save created board to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 4. (final step)
        ContinuationContext context(routine);

        if (routine->errorFlag)
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    //
    routine->start();
}

void MainWindow::onUserToRemoveBoard(const int boardId) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine, boardId]() {
        if (boardView->getBoardId() == boardId) {
            boardView->prepareToClose();

            // wait until boardView->canClose() returns true
            (new PeriodicChecker)->setPeriod(50)->setTimeOut(20000)
                ->setPredicate([this]() {
                    return boardView->canClose();
                })
                ->onPredicateReturnsTrue([routine]() {
                    routine->nextStep();
                })
                ->onTimeOut([routine]() {
                    qWarning().noquote() << "time-out while awaiting BoardView::canClose()";
                    routine->nextStep();
                })
                ->setAutoDelete()->start();
        }
        else {
            routine->nextStep();
        }
    }, this);

    routine->addStep([this, routine, boardId]() {
        if (boardView->getBoardId() == boardId) {
            // close current board
            boardView->loadBoard(
                    -1,
                    // callback
                    [this, routine](bool ok) {
                        ContinuationContext context(routine);
                        if (!ok) {
                            context.setErrorFlag();
                            routine->errorMsg = "could not close current board";
                            return;
                        }

                        boardView->setVisible(false);
                        noBoardOpenSign->setVisible(true);
                    }
            );
        }
        else {
            routine->nextStep();
        }
    }, this);

    routine->addStep([this, routine, boardId]() {
        // remove board from `boardsList`
        ContinuationContext context(routine);
        boardsList->removeBoard(boardId);
    }, this);

    routine->addStep([this, routine]() {
        // save boards ordering
        saveBoardsOrdering([routine](bool ok) {
            ContinuationContext context(routine);
            if (!ok) {
                context.setErrorFlag();
                routine->errorMsg
                        = QString("Could not save boards ordering to DB.\n\n"
                                  "There is unsaved update. See %1")
                          .arg(Services::instance()->getUnsavedUpdateFilePath());
            }
        });
    }, this);

    routine->addStep([this, routine, boardId]() {
        // remove board from DB
        Services::instance()->getCachedDataAccess()->removeBoard(
                boardId,
                // callback
                [routine, boardId](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Could not remove board %1 from DB\n\n"
                                          "There is unsaved update. See %2")
                                  .arg(boardId)
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                    }
                },
                this
        );

    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag)
            showWarningMessageBox(this, " ", routine->errorMsg);
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

    //
    Services::instance()->getCachedDataAccess()->updateBoardNodeProperties(
            currentBoardId, propertiesUpdate,
            // callback
            [this, callback](bool ok) {
                if (!ok) {
                    const auto msg
                            = QString("Could not save board's top-left position to DB.\n\n"
                                      "There is unsaved update. See %1")
                              .arg(Services::instance()->getUnsavedUpdateFilePath());
                    showWarningMessageBox(this, " ", msg);
                }
                callback(ok);
            },
            this
    );
}

void MainWindow::saveBoardsOrdering(std::function<void (bool ok)> callback) {
    BoardsListPropertiesUpdate propertiesUpdate;
    propertiesUpdate.boardsOrdering = boardsList->getBoardsOrder();

    Services::instance()->getCachedDataAccess()->updateBoardsListProperties(
            propertiesUpdate,
            // callback
            [callback](bool ok) {
                if (callback)
                    callback(ok);
            },
            this
    );
}

void MainWindow::showRelationshipTypesDialog() {
    using StringListPair = std::pair<QStringList, QStringList>;

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList relTypes;
        QStringList updatedRelTypes;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get relationship types list
        Services::instance()->getCachedDataAccess()->getUserLabelsAndRelationshipTypes(
                //callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get the list of relationship types";
                    }
                    else {
                        routine->relTypes = labelsAndRelTypes.second;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // show dialog
        auto *dialog = new DialogUserRelationshipTypes(routine->relTypes, this);
        connect(dialog, &QDialog::finished, this, [routine, dialog]() {
            ContinuationContext context(routine);
            routine->updatedRelTypes = dialog->getRelationshipTypesList();
            dialog->deleteLater();

        });
        dialog->open();
    }, this);

    routine->addStep([this, routine]() {
        if (routine->updatedRelTypes == routine->relTypes) {
            routine->nextStep();
            return;
        }

        // write DB
        Services::instance()->getCachedDataAccess()->updateUserRelationshipTypes(
                routine->updatedRelTypes,
                // callback
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = QString("Could not save user-defined relationship types to DB.\n\n"
                                          "There is unsaved update. See %1")
                                  .arg(Services::instance()->getUnsavedUpdateFilePath());
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag)
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}
