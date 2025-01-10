#include <cmath>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QIcon>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include "app_data.h"
#include "main_window.h"
#include "persisted_data_access.h"
#include "services.h"
#include "ui_main_window.h"
#include "utilities/action_debouncer.h"
#include "utilities/async_routine.h"
#include "utilities/fonts_util.h"
#include "utilities/margins_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "widgets/board_view.h"
#include "widgets/boards_list.h"
#include "widgets/dialogs/dialog_user_card_labels.h"
#include "widgets/dialogs/dialog_user_relationship_types.h"
#include "widgets/right_sidebar.h"
#include "widgets/workspace_frame.h"
#include "widgets/workspaces_list.h"

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
                Services::instance()->getAppData()
                        ->updateMainWindowSize(EventSource(this), this->size());
            },
            this
    );

    //
    onStartUp();
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

    checkIsScreenChanged();
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

//        boardsList->setEnabled(false);
        workspacesList->setEnabled(false);
//        boardView->setEnabled(false);
        workspaceFrame->setEnabled(false);

        onUserCloseWindow();

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

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    checkIsScreenChanged();
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
//        boardsList = new BoardsList;
        workspacesList = new WorkspacesList;
//        leftSideBarLayout->addWidget(boardsList);
        leftSideBarLayout->addWidget(workspacesList);
    }

    // set up ui->frameCentralArea
    ui->frameCentralArea->setFrameShape(QFrame::NoFrame);
    {
        auto *vBoxLayout = new QVBoxLayout;
        vBoxLayout->setContentsMargins(0, 0, 0, 0);
        ui->frameCentralArea->setLayout(vBoxLayout);
        {
//            boardView = new BoardView;
//            vBoxLayout->addWidget(boardView);
//            boardView->setVisible(false);
            workspaceFrame = new WorkspaceFrame;
            vBoxLayout->addWidget(workspaceFrame);
            workspaceFrame->setVisible(false);

            noWorkspaceOpenSign = new QLabel("No workspace is open");
            vBoxLayout->addWidget(noWorkspaceOpenSign);
            vBoxLayout->setAlignment(noWorkspaceOpenSign, Qt::AlignCenter);
        }
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
    noWorkspaceOpenSign->setStyleSheet(
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
//    connect(boardsList, &BoardsList::boardSelected,
//            this, [this](int newBoardId, int /*previousBoardId*/) {
//        onBoardSelectedByUser(newBoardId);
//    });

//    connect(boardsList, &BoardsList::userRenamedBoard,
//            this, [this](int boardId, QString name) {
//        BoardNodePropertiesUpdate update;
//        update.name = name;

//        Services::instance()->getAppData()->updateBoardNodeProperties(
//                EventSource(this), boardId, update);
//    });

//    connect(boardsList, &BoardsList::userToCreateNewBoard, this, [this]() {
//        onUserToCreateNewBoard();
//    });

//    connect(boardsList, &BoardsList::userToRemoveBoard, this, [this](int boardId) {
//        const auto r = QMessageBox::question(
//                this, " ",
//                QString("Delete the board \"%1\"?").arg(boardsList->boardName(boardId)));
//        if (r != QMessageBox::Yes)
//            return;

//        onUserToRemoveBoard(boardId);
//    });

//    connect(boardsList, &BoardsList::boardsOrderChanged, this, [this](QVector<int> /*boardIds*/) {
//        saveBoardsOrdering();
//    });

    // workspacesList
    connect(workspacesList, &WorkspacesList::workspaceSelected,
            this, [this](int newWorkspaceId, int /*previousWorkspaceId*/) {
        onWorkspaceSelectedByUser(newWorkspaceId);
    });

    connect(workspacesList, &WorkspacesList::userRenamedWorkspace,
            this, [this](int workspaceId, QString name) {
        onUserRenamedWorkspace(workspaceId, name);
    });

    connect(workspacesList, &WorkspacesList::userToCreateNewWorkspace, this, [this]() {
        onUserToCreateNewWorkspace();
    });

    connect(workspacesList, &WorkspacesList::userToRemoveWorkspace, this, [this](int workspaceId) {
        onUserToRemoveWorkspace(workspaceId);
    });

    connect(workspacesList, &WorkspacesList::workspacesOrderChanged,
            this, [this](QVector<int> /*workspaceIds*/) {
        saveWorkspacesOrdering();
    });

    // boardView
//    connect(boardView, &BoardView::openRightSideBar, this, [this]() {
//        ui->frameRightSideBar->setVisible(true);
//    });

    // boardView
    connect(workspaceFrame, &WorkspaceFrame::openRightSidebar, this, [this]() {
        ui->frameRightSideBar->setVisible(true);
    });

    // rightSidebar
    connect(rightSidebar, &RightSidebar::closeRightSidebar, this, [this]() {
        ui->frameRightSideBar->setVisible(false);
//        boardView->showButtonRightSidebar();
        workspaceFrame->showButtonRightSidebar();
    });
}

void MainWindow::setUpMainMenu() {
    {
        auto *submenu = mainMenu->addMenu("Graph");
        {
            submenu->addAction("Labels...", this, [this]() {
                onUserToSetCardLabelsList();
            });
            submenu->addAction("Relationship Types...", this, [this]() {
                onUserToSetRelationshipTypesList();
            });
        }
    }
    {
        auto *submenu = mainMenu->addMenu("View");
        {
            auto *action = submenu->addAction("Zoom In", this, [this]() {
//                boardView->applyZoomAction(BoardView::ZoomAction::ZoomIn);
                // todo ...
            });
            action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
            this->addAction(action); // without this, the shortcut won't work
        }
        {
            auto *action = submenu->addAction("Zoom Out", this, [this]() {
//                boardView->applyZoomAction(BoardView::ZoomAction::ZoomOut);
                // todo ...
            });
            action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
            this->addAction(action); // without this, the shortcut won't work
        }
        {
            auto *action = submenu->addAction("Reset Zoom", this, [this]() {
//                boardView->applyZoomAction(BoardView::ZoomAction::ResetZoom);
                // todo ...
            });
            action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
            this->addAction(action); // without this, the shortcut won't work
        }
    }
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

void MainWindow::onStartUp() {
    {
        const auto sizeOpt
                = Services::instance()->getAppDataReadonly()->getMainWindowSize();
        if (sizeOpt.has_value())
            resize(sizeOpt.value());
    }

//    boardsList->setEnabled(false);
    workspacesList->setEnabled(false);
//    boardView->setEnabled(false);
    workspaceFrame->setEnabled(false);

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        WorkspacesListProperties workspacesListProperties;
        QHash<int, Workspace> workspaces;
//        BoardsListProperties boardsListProperties;
//        QHash<int, QString> boardsIdToName;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // get workspaces-list properties
        Services::instance()->getAppDataReadonly()->getWorkspacesListProperties(
                [routine](bool ok, WorkspacesListProperties properties) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        routine->errorMsg
                                = "Could not get workspaces list properties. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        routine->workspacesListProperties = properties;
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // get workspaces
        Services::instance()->getAppDataReadonly()->getWorkspaces(
                [routine](bool ok, const QHash<int, Workspace> &workspaces) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        routine->errorMsg
                                = "Could not get data of workspaces. See logs for details.";
                        context.setErrorFlag();
                    }
                    else {
                        routine->workspaces = workspaces;
                    }
                },
                this
        );
    }, this);

//    routine->addStep([this, routine]() {
//        // get boards-list properties
//        Services::instance()->getAppDataReadonly()->getBoardsListProperties(
//                [routine](bool ok, BoardsListProperties properties) {
//                    ContinuationContext context(routine);

//                    if (!ok) {
//                        routine->errorMsg
//                                = "Could not get boards list properties. See logs for details.";
//                        context.setErrorFlag();
//                    }
//                    else {
//                        routine->boardsListProperties = properties;
//                    }
//                },
//                this
//        );
//    }, this);

//    routine->addStep([this, routine]() {
//        // get all board IDs
//        Services::instance()->getAppDataReadonly()->getBoardIdsAndNames(
//                [routine](bool ok, const QHash<int, QString> &idToName) {
//                    ContinuationContext context(routine);

//                    if (!ok) {
//                        routine->errorMsg
//                                = "Could not get the list of boards. See logs for details.";
//                        context.setErrorFlag();
//                    }
//                    else {
//                        routine->boardsIdToName = idToName;
//                    }
//                },
//                this
//        );
//    }, this);

    routine->addStep([this, routine]() {
        // populate `boardsList`
//        boardsList->resetBoards(
//                routine->boardsIdToName, routine->boardsListProperties.boardsOrdering);

        // populate `workspacesList`
        QHash<int, QString> workspacesIdToName;
        for (auto it = routine->workspaces.constBegin(); it != routine->workspaces.constEnd(); ++it)
            workspacesIdToName.insert(it.key(), it.value().name);

        workspacesList->resetWorkspaces(
                workspacesIdToName, routine->workspacesListProperties.workspacesOrdering);

        // load last-opened board
//        const int lastOpenedBoardId = routine->boardsListProperties.lastOpenedBoard; // can be -1
//        if (routine->boardsIdToName.contains(lastOpenedBoardId)) {
//            boardView->setVisible(true); // (this must be before calling boardView->loadBoard())
//            noBoardOpenSign->setVisible(false);

//            boardView->loadBoard(
//                    lastOpenedBoardId,
//                    // callback
//                    [this, routine, lastOpenedBoardId](bool loadOk, bool highlightedCardIdChanged) {
//                        ContinuationContext context(routine);

//                        if (loadOk) {
//                            boardsList->setSelectedBoardId(lastOpenedBoardId);
//                        }
//                        else {
//                            routine->errorMsg
//                                    = QString("Could not load board %1").arg(lastOpenedBoardId);
//                            context.setErrorFlag();
//                        }

//                        if (highlightedCardIdChanged) {
//                            // call AppData
//                            constexpr int highlightedCardId = -1;
//                            Services::instance()->getAppData()
//                                    ->setHighlightedCardId(EventSource(this), highlightedCardId);
//                        }
//                    }
//            );
//        }
//        else {
//            if (lastOpenedBoardId != -1) {
//                qWarning().noquote()
//                        << QString("last-opened board (ID=%1) does not exist")
//                           .arg(lastOpenedBoardId);
//            }
//            routine->nextStep();
//        }
        routine->nextStep();

    }, this);

    routine->addStep([this, routine]() {
        // (final step)
        ContinuationContext context(routine);

//        boardsList->setEnabled(true);
        workspacesList->setEnabled(true);
//        boardView->setEnabled(true);
        workspaceFrame->setEnabled(true);

        if (routine->errorFlag)
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::onWorkspaceSelectedByUser(const int workspaceId) {
    auto *routine = new AsyncRoutine;

    routine->addStep([this, routine]() {
        saveTopLeftPosAndZoomRatioOfCurrentBoard();
        saveLastOpenedBoardOfCurrentWorkspace();
        workspaceFrame->prepareToClose();

        // wait until workspaceFrame->canClose() returns true
        (new PeriodicChecker)->setPeriod(50)->setTimeOut(20000)
            ->setPredicate([this]() {
                return workspaceFrame->canClose();
            })
            ->onPredicateReturnsTrue([routine]() {
                routine->nextStep();
            })
            ->onTimeOut([routine]() {
                qWarning().noquote() << "time-out while awaiting WorkspaceFrame::canClose()";
                routine->nextStep();
            })
            ->setAutoDelete()->start();
    }, this);

    routine->addStep([this, routine, workspaceId]() {
        // load `workspaceId`
        noWorkspaceOpenSign->setVisible(false);
        workspaceFrame->setVisible(true);

        workspaceFrame->loadWorkspace(
                workspaceId,
                // callback:
                [this, routine, workspaceId](bool loadOk, bool highlightedCardIdChanged) {
                    if (!loadOk) {
                        QMessageBox::warning(
                                this, " ", QString("Could not load workspace %1").arg(workspaceId));
                    }
                    if (highlightedCardIdChanged) {
                        // call AppData
                        constexpr int highlightedCardId = -1;
                        Services::instance()->getAppData()
                                ->setHighlightedCardId(EventSource(this), highlightedCardId);
                    }
                    routine->nextStep();
                });
    }, this);

    routine->start();
}

void MainWindow::onUserToCreateNewWorkspace() {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newWorkspaceId {-1};
        Workspace workspaceData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // 1. get new workspace ID (use board ID)
        Services::instance()->getAppDataReadonly()->requestNewBoardId(
                //callback
                [routine](std::optional<int> newId) {
                    ContinuationContext context(routine);

                    if (!newId.has_value()) {
                        context.setErrorFlag();
                        routine->errorMsg = "Could not get new workspace ID. See logs for details.";
                    }
                    else {
                        routine->newWorkspaceId = newId.value();
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 2. add to `workspacesList`
        ContinuationContext context(routine);

        routine->workspaceData.name = "new workspace";

        workspacesList->addWorkspace(routine->newWorkspaceId, routine->workspaceData.name);
        workspacesList->startEditWorkspaceName(routine->newWorkspaceId);

        saveWorkspacesOrdering();
    }, this);

    routine->addStep([this, routine]() {
        // 3. call AppData
        ContinuationContext context(routine);

        Services::instance()->getAppData()->createNewWorkspaceWithId(
                EventSource(this), routine->newWorkspaceId, routine->workspaceData);
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

void MainWindow::onUserRenamedWorkspace(const int workspaceId, const QString &newName) {
    WorkspaceNodePropertiesUpdate update;
    update.name = newName;

    Services::instance()->getAppData()->updateWorkspaceNodeProperties(
            EventSource(this), workspaceId, update);

    if (workspaceFrame->getWorkspaceId() == workspaceId)
        workspaceFrame->changeWorkspaceName(newName);
}

void MainWindow::onUserToRemoveWorkspace(const int workspaceIdToRemove) {
    if (workspaceIdToRemove == -1)
        return;

    // show confirmation message box
    const auto r = QMessageBox::question(
            this, "Please Confirm",
            QString("Remove the workspace \"%1\"?"
                    " All boards in the workspaces will be removed.")
                .arg(workspacesList->workspaceName(workspaceIdToRemove)));
    if (r != QMessageBox::Yes)
        return;

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QSet<int> originalBoardIdsOfWorkspace;
        bool toCloseWorkspace {false};
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine, workspaceIdToRemove]() {
        // get the boards of `workspaceIdToRemove`
        Services::instance()->getAppDataReadonly()->getWorkspaces(
                [routine, workspaceIdToRemove](bool ok, const QHash<int, Workspace> &workspacesData) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }

                    if (!workspacesData.contains(workspaceIdToRemove)) {
                        qWarning().noquote()
                                << QString("could not get data of workspace %1")
                                   .arg(workspaceIdToRemove);
                        context.setErrorFlag();
                        return;
                    }

                    routine->originalBoardIdsOfWorkspace
                            = workspacesData.value(workspaceIdToRemove).boardIds;
                },
                this
        );
    }, this);

    routine->addStep([this, routine, workspaceIdToRemove]() {
        // remove the workspace
        ContinuationContext context(routine);

        workspacesList->removeWorkspace(workspaceIdToRemove);
        saveWorkspacesOrdering();
        Services::instance()->getAppData()->removeWorkspace(
                    EventSource(this), workspaceIdToRemove, routine->originalBoardIdsOfWorkspace);
    }, this);

    routine->addStep([this, routine, workspaceIdToRemove]() {
        // determine whether to close workspace
        ContinuationContext context(routine);

        routine->toCloseWorkspace = (workspaceFrame->getWorkspaceId() == workspaceIdToRemove);
        if (routine->toCloseWorkspace)
            workspacesList->setSelectedWorkspaceId(-1);
    }, this);

    routine->addStep([this, routine]() {
        // prepare to close `workspaceFrame`
        if (!routine->toCloseWorkspace) {
            routine->nextStep();
            return;
        }

        saveLastOpenedBoardOfCurrentWorkspace();
        workspaceFrame->setVisible(true);
        noWorkspaceOpenSign->setVisible(false);
        workspaceFrame->prepareToClose();

        // wait until workspaceFrame->canClose() returns true
        (new PeriodicChecker)->setPeriod(50)->setTimeOut(20000)
            ->setPredicate([this]() {
                return workspaceFrame->canClose();
            })
            ->onPredicateReturnsTrue([routine]() {
                routine->nextStep();
            })
            ->onTimeOut([routine]() {
                qWarning().noquote() << "time-out while awaiting WorkspaceFrame::canClose()";
                routine->nextStep();
            })
            ->setAutoDelete()->start();
    }, this);

    routine->addStep([this, routine]() {
        // close workspace
        if (!routine->toCloseWorkspace) {
            routine->nextStep();
            return;
        }

        workspaceFrame->loadWorkspace(
                -1,
                // callback
                [this, routine](bool ok, bool highlightedCardIdChanged) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("Could not close workspace");
                    }

                    if (highlightedCardIdChanged) {
                        constexpr int highlightedCardId = -1;
                        Services::instance()->getAppData()
                                ->setHighlightedCardId(EventSource(this), highlightedCardId);
                    }
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        // show no-workspace sign if no workspace is open
        ContinuationContext context(routine);
        if (workspacesList->selectedWorkspaceId() == -1) {
            workspaceFrame->setVisible(false);
            noWorkspaceOpenSign->setVisible(true);
        }
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::onUserToSetCardLabelsList() {
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
        ContinuationContext context(routine);

        if (routine->updatedLabels == routine->labels)
            return;

        Services::instance()->getAppData()
                ->updateUserCardLabels(EventSource(this), routine->updatedLabels);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::onUserToSetRelationshipTypesList() {
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
        ContinuationContext context(routine);

        if (routine->updatedRelTypes == routine->relTypes)
            return;

        Services::instance()->getAppData()
                ->updateUserRelationshipTypes(EventSource(this), routine->updatedRelTypes);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void MainWindow::onUserCloseWindow() {
    saveWindowSizeDebounced->actNow();

    //
    auto *routine = new AsyncRoutineWithErrorFlag;

    routine->addStep([this, routine]() {
        // save last-opened board ID
        ContinuationContext context(routine);

//        BoardsListPropertiesUpdate propertiesUpdate;
//        propertiesUpdate.lastOpenedBoard = boardsList->selectedBoardId();

//        Services::instance()->getAppData()
//                ->updateBoardsListProperties(EventSource(this), propertiesUpdate);
    }, this);

    routine->addStep([this, routine]() {
        // save
        ContinuationContext context(routine);

        saveTopLeftPosAndZoomRatioOfCurrentBoard();
        saveLastOpenedBoardOfCurrentWorkspace();
    }, this);

    routine->addStep([this, routine]() {
        Services::instance()->finalize(
                6000, // timeout (ms)
                [this, routine](bool timedOut) {
                    ContinuationContext context(routine);

                    if (timedOut) {
                        QMessageBox::warning(
                                this, " ",
                                "Time-out while awaiting DB-access operations to finish.");
                        context.setErrorFlag();
                    }
                },
                this
        );
    }, this);

//    routine->addStep([this, routine]() {
//        ContinuationContext context(routine);

//        if (routine->hasUnsavedUpdate) {
//            const auto r
//                    = QMessageBox::question(this, " ", "There is unsaved update. Exit anyway?");
//            if (r != QMessageBox::Yes) {
//                context.setErrorFlag();
//                return;
//            }
//        }
//    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag) {
//            boardsList->setEnabled(true);
            workspacesList->setEnabled(true);
//            boardView->setEnabled(true);
            workspaceFrame->setEnabled(true);
            closingState = ClosingState::NotClosing;
        }
        else {
            closingState = ClosingState::CloseNow;
            close();
        }
    }, this);

    routine->start();
}

void MainWindow::saveTopLeftPosAndZoomRatioOfCurrentBoard() {
    const int currentBoardId = workspaceFrame->getCurrentBoardId();
    if (currentBoardId == -1)
        return;

    BoardNodePropertiesUpdate propertiesUpdate;
    {
        propertiesUpdate.topLeftPos = workspaceFrame->getBoardViewTopLeftPos();
        propertiesUpdate.zoomRatio = workspaceFrame->getBoardViewZoomRatio();
    }
    Services::instance()->getAppData()->updateBoardNodeProperties(
                EventSource(this), currentBoardId, propertiesUpdate);
}

void MainWindow::saveLastOpenedBoardOfCurrentWorkspace() {
    const int workspaceId = workspaceFrame->getWorkspaceId();
    if (workspaceId == -1)
        return;

    WorkspaceNodePropertiesUpdate update;
    {
        update.lastOpenedBoardId = workspaceFrame->getCurrentBoardId();
    }
    Services::instance()->getAppData()->updateWorkspaceNodeProperties(
            EventSource(this), workspaceId, update);
}

void MainWindow::saveWorkspacesOrdering() {
    const QVector<int> workspaceIds = workspacesList->getWorkspaceIds();

    WorkspacesListPropertiesUpdate update;
    {
        update.workspacesOrdering = workspaceIds;
    }
    Services::instance()->getAppData()->updateWorkspacesListProperties(EventSource(this), update);
}

void MainWindow::checkIsScreenChanged() {
    if (const QScreen *newScreen = screen(); newScreen != currentScreen) {
        qInfo() << "screen changed";
        currentScreen = newScreen;

        const double factor = fontSizeScaleFactor(this);
        qInfo() << "font size scale factor:" << factor;
        qInfo() << "screen logical DPI:" << screen()->logicalDotsPerInch();

        Services::instance()->getAppData()->updateFontSizeScaleFactor(this, factor);
    }
}
