#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QIcon>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include "app_data_readonly.h"
#include "app_events_handler.h"
#include "main_window.h"
#include "persisted_data_access.h"
#include "services.h"
#include "ui_main_window.h"
#include "utilities/action_debouncer.h"
#include "utilities/async_routine.h"
#include "utilities/margins_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "widgets/board_view.h"
#include "widgets/boards_list.h"
#include "widgets/dialogs/dialog_user_card_labels.h"
#include "widgets/dialogs/dialog_user_relationship_types.h"
#include "widgets/dialogs/dialog_settings.h"
#include "widgets/right_sidebar.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow)
        , mainMenu(new QMenu(this)) {
    ui->setupUi(this);
    setUpWidgets();
    setUpConnections();
    setUpMainMenu();

    //
    saveWindowSizeDebounced = new ActionDebouncer(
            1000, ActionDebouncer::Option::Delay,
            [this]() {
                Services::instance()->getAppEventsHandler()->updatedMainWindowSize(
                        EventSource(this), this->size(),
                        // callbackPersistResult
                        [](bool /*ok*/) {},
                        this
                );
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

    // prevent left & right sidebars from resizing when the window resizes
    ui->splitter->setStretchFactor(0, 0); // (index, factor)
    ui->splitter->setStretchFactor(1, 1);
    ui->splitter->setStretchFactor(2, 0);

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
    ui->frameCentralArea->setFrameShape(QFrame::NoFrame);
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

    // set up ui->frameRightSideBar
    ui->frameRightSideBar->setVisible(false);
    ui->frameRightSideBar->setFrameShape(QFrame::NoFrame);
    {
        auto *layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        ui->frameRightSideBar->setLayout(layout);

        rightSidebar = new RightSidebar;
        layout->addWidget(rightSidebar);
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

        Services::instance()->getAppEventsHandler()->updatedBoardNodeProperties(
                EventSource(this),
                boardId, update,
                // callbackPersistResult
                [](bool /*ok*/) {},
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
        saveBoardsOrdering([](bool /*ok*/) {});
    });

    // boardView
    connect(boardView, &BoardView::openRightSideBar, this, [this]() {
        ui->frameRightSideBar->setVisible(true);
    });

    // rightSidebar
    connect(rightSidebar, &RightSidebar::closeRightSidebar, this, [this]() {
        ui->frameRightSideBar->setVisible(false);
        boardView->rightSideBarClosed();
    });
}

void MainWindow::setUpMainMenu() {
    {
        auto *submenu = mainMenu->addMenu("Graph");
        {
            submenu->addAction("Labels...", this, [this]() {
                showCardLabelsDialog();
            });
            submenu->addAction("Relationship Types...", this, [this]() {
                showRelationshipTypesDialog();
            });
        }
    }
//    {
//        auto *action = mainMenu->addAction("Settings", this, [this]() {
//            showSettingsDialog();
//        });
//        action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));
//        this->addAction(action); // without this, the shortcut won't work
//    }
    mainMenu->addSeparator();
    {
        auto *action = mainMenu->addAction("Quit", this, [this]() {
            close();
        });
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
        this->addAction(action); // without this, the shortcut won't work
    }
}

void MainWindow::onShownForFirstTime() {
    {
        // set initial widths of left sidebar & central area, assuming right sidebar is hidden
        const QList<int> sizes = ui->splitter->sizes();
                // [0]: left sidebar, [1]: central area, [2]: right sidebar (hidden)
        if (sizes.count() == 3) {
            const int totalWidth = sizes.at(0) + sizes.at(1);
            if (totalWidth >= leftSideBarWidthDefault * 2) {
                ui->splitter->setSizes({
                    leftSideBarWidthDefault,
                    totalWidth - leftSideBarWidthDefault,
                    rightSideBarWidthDefault // right sidebar will have this size when first shown
                });
            }
        }
        else {
            Q_ASSERT(false);
        }
    }
}

void MainWindow::startUp() {
    {
        const auto sizeOpt
                = Services::instance()->getAppDataReadonly()->getMainWindowSize();
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
        Services::instance()->getAppDataReadonly()->getBoardsListProperties(
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
        Services::instance()->getAppDataReadonly()->getBoardIdsAndNames(
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
        qInfo().noquote() << "awaiting all saving operations to finish";
        (new PeriodicChecker)->setPeriod(20)->setTimeOut(6000)
                ->setPredicate([]() {
                    return !Services::instance()
                            ->getPersistedDataAccessHasWriteRequestInProgress();
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
        BoardsListPropertiesUpdate propertiesUpdate;
        propertiesUpdate.lastOpenedBoard = boardsList->selectedBoardId();

        Services::instance()->getAppEventsHandler()->updatedBoardsListProperties(
                EventSource(this),
                propertiesUpdate,
                // callbackPersistResult
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        routine->hasUnsavedUpdate = true;
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
        Services::instance()->getAppDataReadonly()->requestNewBoardId(
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
            if (!ok)
                context.setErrorFlag();
        });
    }, this);

    routine->addStep([this, routine]() {
        //
        Services::instance()->getAppEventsHandler()->createdNewBoard(
                EventSource(this),
                routine->newBoardId, routine->boardData,
                // callbackPersistResult
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
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
            if (!ok)
                context.setErrorFlag();
        });
    }, this);

    routine->addStep([this, routine, boardId]() {
        Services::instance()->getAppEventsHandler()->removedBoard(
                EventSource(this),
                boardId,
                // callbackPersistResult
                [routine, boardId](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                this
        );

    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
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
    Services::instance()->getAppEventsHandler()->updatedBoardNodeProperties(
            EventSource(this),
            currentBoardId, propertiesUpdate,
            // callbackPersistResult
            [callback](bool ok) {
                callback(ok);
            },
            this
    );
}

void MainWindow::saveBoardsOrdering(std::function<void (bool ok)> callback) {
    BoardsListPropertiesUpdate propertiesUpdate;
    propertiesUpdate.boardsOrdering = boardsList->getBoardsOrder();

    Services::instance()->getAppEventsHandler()->updatedBoardsListProperties(
            EventSource(this),
            propertiesUpdate,
            // callbackPersistResult
            [callback](bool ok) {
                if (callback)
                    callback(ok);
            },
            this
    );
}

void MainWindow::showCardLabelsDialog() {
    using StringListPair = std::pair<QStringList, QStringList>;

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QStringList labels;
        QStringList updatedLabels;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get relationship types list
        Services::instance()->getAppDataReadonly()->getUserLabelsAndRelationshipTypes(
                //callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get the list of user-defined card labels";
                    }
                    else {
                        routine->labels = labelsAndRelTypes.first;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // show dialog
        auto *dialog = new DialogUserCardLabels(routine->labels, this);
        connect(dialog, &QDialog::finished, this, [routine, dialog]() {
            ContinuationContext context(routine);
            routine->updatedLabels = dialog->getLabelsList();
            dialog->deleteLater();
        });
        dialog->open();
    }, this);

    routine->addStep([this, routine]() {
        if (routine->updatedLabels == routine->labels) {
            routine->nextStep();
            return;
        }

        //
        Services::instance()->getAppEventsHandler()->updatedUserCardLabels(
                EventSource(this),
                routine->updatedLabels,
                // callbackPersistResult
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
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
        Services::instance()->getAppDataReadonly()->getUserLabelsAndRelationshipTypes(
                //callback
                [routine](bool ok, const StringListPair &labelsAndRelTypes) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg
                                = "Could not get the list of user-defined relationship types";
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

        //
        Services::instance()->getAppEventsHandler()->updatedUserRelationshipTypes(
                EventSource(this),
                routine->updatedRelTypes,
                // callbackPersistResult
                [routine](bool ok) {
                    ContinuationContext context(routine);
                    if (!ok)
                        context.setErrorFlag();
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::showSettingsDialog() {
    auto *dialog = new DialogSettings(this);
    connect(dialog, &QDialog::finished, this, [dialog](int /*result*/) {
        dialog->deleteLater();
    });
    dialog->open();
}
