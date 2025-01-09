#include <QDebug>
#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include "app_data.h"
#include "app_data_readonly.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "widgets/board_view.h"
#include "workspace_frame.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

namespace {
constexpr char keyBoardId[] = "boardId";
}

WorkspaceFrame::WorkspaceFrame(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpConnections();
    setUpBoardTabContextMenu();
}

void WorkspaceFrame::loadWorkspace(
        const int workspaceIdToLoad, std::function<void (bool, bool)> callback) {
    if (workspaceId == workspaceIdToLoad) {
        callback(true, false);
        return;
    }

    //
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        bool highlightedCardIdChanged {false};
        Workspace workspaceData;
        QHash<int, QString> boardIdToName;
        int boardIdToOpen {-1};
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine]() {
        // 1. prepare to close `boardView`
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

    routine->addStep([this, routine]() {
        // 2. close `boardView`
        boardView->setVisible(true);
        boardView->loadBoard(-1, [routine](bool loadOk, bool highlightedCardIdChanged) {
            ContinuationContext context(routine);
            if (!loadOk) {
                qWarning().noquote() << "could not close the board";
                context.setErrorFlag();
            }
            routine->highlightedCardIdChanged |= highlightedCardIdChanged;
        });
    }, this);

    routine->addStep([this, routine]() {
        // 3. clear `boardsTabBar`
        ContinuationContext context(routine);
        while (boardsTabBar->count() != 0)
            boardsTabBar->removeTab(0);
    }, this);

    routine->addStep([this, routine, workspaceIdToLoad]() {
        // 4. get workspace data
        if (workspaceIdToLoad == -1) {
            routine->nextStep();
            return;
        }

        Services::instance()->getAppDataReadonly()->getWorkspaces(
                [routine, workspaceIdToLoad](bool ok, const QHash<int, Workspace> &workspacesData) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }

                    if (!workspacesData.contains(workspaceIdToLoad)) {
                        qWarning().noquote()
                                << QString("could not get data of workspace %1")
                                   .arg(workspaceIdToLoad);
                        context.setErrorFlag();
                        return;
                    }

                    routine->workspaceData = workspacesData.value(workspaceIdToLoad);
                },
                this
        );
    }, this);

    routine->addStep([this, routine, workspaceIdToLoad]() {
        // 5. get board names
        if (workspaceIdToLoad == -1) {
            routine->nextStep();
            return;
        }

        Services::instance()->getAppDataReadonly()->getBoardIdsAndNames(
                [routine](bool ok, const QHash<int, QString> &boardIdToName) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        return;
                    }

                    const QSet<int> boardIds = routine->workspaceData.boardIds;
                    for (const int id: boardIds) {
                        if (!boardIdToName.contains(id)) {
                            qWarning().noquote()
                                    << QString("could not get the name of board %1").arg(id);
                            context.setErrorFlag();
                            return;
                        }
                        routine->boardIdToName.insert(id, boardIdToName.value(id));
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // 6. populate `boardsTabBar` and determine `routine->boardIdToOpen`
        ContinuationContext context(routine);

        const QVector<int> sortedBoardIds = sortByOrdering(
                keySet(routine->boardIdToName), routine->workspaceData.boardsOrdering, false);
        for (const int boardId: sortedBoardIds) {
            const QString boardName = routine->boardIdToName.value(boardId);

            const int tabIndex = boardsTabBar->addTab(boardName);
            boardsTabBar->setTabData(tabIndex, QJsonObject {{keyBoardId, boardId}});
        }

        //
        if (sortedBoardIds.isEmpty()) {
            routine->boardIdToOpen = -1;
        }
        else {
            routine->boardIdToOpen = sortedBoardIds.contains(routine->workspaceData.lastOpenedBoardId)
                    ? routine->workspaceData.lastOpenedBoardId
                    : sortedBoardIds.at(0);
        }
    }, this);

    routine->addStep([this, routine]() {
        // 7. open board
        if (routine->boardIdToOpen == -1) {
            noBoardSign->setVisible(true);
            boardView->setVisible(false);
            routine->nextStep();
            return;
        }

        noBoardSign->setVisible(false);
        boardView->loadBoard(
                routine->boardIdToOpen,
                // callback:
                [routine](bool ok, bool highlightedCardIdChanged) {
                    ContinuationContext context(routine);

                    if (!ok) {
                        qWarning().noquote()
                                << QString("could not load board %1").arg(routine->boardIdToOpen);
                        context.setErrorFlag();
                    }
                    routine->highlightedCardIdChanged |= highlightedCardIdChanged;
                }
        );
    }, this);

    routine->addStep([this, workspaceIdToLoad, routine, callback]() {
        // final step
        ContinuationContext context(routine);

        if (!routine->errorFlag) {
            workspaceId = workspaceIdToLoad;
            workspaceName = routine->workspaceData.name;
            workspaceToolBar->setWorkspaceName(routine->workspaceData.name);
        }

        callback(!routine->errorFlag, routine->highlightedCardIdChanged);
    }, this);

    routine->start();
}

void WorkspaceFrame::showButtonRightSidebar() {
    workspaceToolBar->showButtonOpenRightSidebar();
}

void WorkspaceFrame::prepareToClose() {
    // todo ...


}

bool WorkspaceFrame::canClose() const {
    return true; // [temp]


}

void WorkspaceFrame::setUpWidgets() {
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    {
        workspaceToolBar = new WorkspaceToolBar;
        layout->addWidget(workspaceToolBar);

        //
        auto *hLayout = new QHBoxLayout;
        layout->addLayout(hLayout);
        hLayout->setContentsMargins(0, 0, 0, 0);
        hLayout->setSpacing(0);
        {
            boardsTabBar = new QTabBar;
            hLayout->addWidget(boardsTabBar, 0, Qt::AlignBottom);
            boardsTabBar->setExpanding(false);
            boardsTabBar->setMovable(true);
            boardsTabBar->setContextMenuPolicy(Qt::CustomContextMenu);

            //
            auto *buttonAddBoard = new QToolButton;
            hLayout->addWidget(buttonAddBoard);
            buttonAddBoard->setIcon(QIcon(":/icons/add_black_24"));
            buttonAddBoard->setIconSize({20, 20});
            buttonAddBoard->setToolTip("Add Board");
            connect(buttonAddBoard, &QToolButton::clicked, this, [this]() {
                onUserToAddBoard();
            });
            buttonAddBoard->setStyleSheet(
                    "QToolButton {"
                    "  margin: 2px 2px 0 0;" // ^>v<
                    "  padding: 2px;"
                    "  border: none;"
                    "  border-bottom: 1px solid #a0a0a0;"
                    "  background: transparent;"
                    "}"
                    "QToolButton:hover {"
                    "  background: #d0d0d0;"
                    "}");
        }

        //
        boardView = new BoardView;
        layout->addWidget(boardView);
        boardView->setVisible(false);

        //
        noBoardSign = new QLabel("No board in this workspace");
        layout->addWidget(noBoardSign, 0, Qt::AlignCenter);
        noBoardSign->setStyleSheet(
                "QLabel {"
                "  color: #808080;"
                "  font-size: 14pt;"
                "  font-weight: bold;"
                "}");
    }
}

void WorkspaceFrame::setUpConnections() {
    connect(workspaceToolBar, &WorkspaceToolBar::openRightSidebar, this, [this]() {
        emit openRightSidebar();
    });

    // `boardsTabBar`
    connect(boardsTabBar, &QTabBar::customContextMenuRequested, this, [this](const QPoint &pos) {
        if (boardsTabBar->currentIndex() == -1)
            return;
        if (boardsTabBar->tabAt(pos) != boardsTabBar->currentIndex())
            return;
        boardTabContextMenu->popup(boardsTabBar->mapToGlobal(pos));
    });
}

void WorkspaceFrame::setUpBoardTabContextMenu() {
    boardTabContextMenu = new QMenu(this);
    {
        auto *action = boardTabContextMenu->addAction(
                QIcon(":/icons/edit_square_black_24"), "Rename");
        connect(action, &QAction::triggered, this, [this]() {
            if (boardsTabBar->currentIndex() == -1)
                return;
            const int boardId
                    = boardsTabBar->tabData(boardsTabBar->currentIndex())
                      .toJsonObject().value(keyBoardId).toInt(-1);
            if (boardId == -1)
                return;
            const QString boardName = boardsTabBar->tabText(boardsTabBar->currentIndex());
            onUserToRenameBoard(boardId, boardName);
        });
    }
}

void WorkspaceFrame::onUserToAddBoard() {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        int newBoardId {-1};
        Board newBoardData;
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // request new Board ID
        Services::instance()->getAppData()->requestNewBoardId(
                [routine](std::optional<int> newId) {
                    ContinuationContext context(routine);
                    if (!newId.has_value()) {
                        context.setErrorFlag();
                        routine->errorMsg = "Failed to request new board ID";
                    }
                    else {
                        routine->newBoardId = newId.value();
                    }
                },
                this
        );
    }, this);

    routine->addStep([this, routine]() {
        // create new Board
        ContinuationContext context(routine);

        Q_ASSERT(routine->newBoardId != -1);
        Q_ASSERT(workspaceId != -1);

        routine->newBoardData.name = "New Board";
        Services::instance()->getAppData()->createNewBoardWithId(
                EventSource(this), routine->newBoardId, routine->newBoardData, workspaceId);
    }, this);

    routine->addStep([this, routine]() {
        // prepare to close `boardView`
        boardView->setVisible(true);
        noBoardSign->setVisible(false);
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

    routine->addStep([this, routine]() {
        // load board
        boardView->loadBoard(
                routine->newBoardId,
                // callback
                [this, routine](bool ok, bool highlightedCardIdChanged) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("Could not load board %1").arg(routine->newBoardId);
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
        // add to `boardsTabBar`
        ContinuationContext context(routine);

        const int tabIndex = boardsTabBar->addTab(routine->newBoardData.name);
        boardsTabBar->setTabData(tabIndex, QJsonObject {{keyBoardId, routine->newBoardId}});
        boardsTabBar->setCurrentIndex(tabIndex);
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);
    }, this);

    routine->start();
}

void WorkspaceFrame::onUserToRenameBoard(const int boardId, const QString &originalName) {
    qDebug() << QString("rename board %1 (%2)").arg(boardId).arg(originalName);




}

//========

WorkspaceToolBar::WorkspaceToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    setUpWorkspaceSettingsMenu();

    //
    {
        labelWorkspaceName = new QLabel;
        hLayout->addWidget(labelWorkspaceName);

        labelWorkspaceName->setStyleSheet(
                "QLabel {"
                "  font-size: 12pt;"
                "  margin-left: 4px"
                "}");
    }
    hLayout->addStretch();
    {
        buttonWorkspaceSettings = new QToolButton;
        hLayout->addWidget(buttonWorkspaceSettings);
        hLayout->setAlignment(buttonWorkspaceSettings, Qt::AlignVCenter);

        buttonWorkspaceSettings->setIcon(QIcon(":/icons/more_vert_24"));
        buttonWorkspaceSettings->setIconSize({24, 24});
        buttonWorkspaceSettings->setToolTip("Workspace Settings");

        connect(buttonWorkspaceSettings, &QToolButton::clicked, this, [this]() {
            const QSize s = buttonWorkspaceSettings->size();
            const QPoint bottomRight = buttonWorkspaceSettings->mapToGlobal({s.width(), s.height()});
            workspaceSettingsMenu->popup({
                bottomRight.x() - workspaceSettingsMenu->sizeHint().width(),
                bottomRight.y()
            });
        });
    }
    {
        buttonOpenRightSidebar = new QToolButton;
        hLayout->addWidget(buttonOpenRightSidebar);
        hLayout->setAlignment(buttonOpenRightSidebar, Qt::AlignVCenter);

        buttonOpenRightSidebar->setIcon(QIcon(":/icons/open_right_panel_24"));
        buttonOpenRightSidebar->setIconSize({24, 24});
        buttonOpenRightSidebar->setToolTip("Open Right Side-Bar");

        connect(buttonOpenRightSidebar, &QToolButton::clicked, this, [this]() {
            emit openRightSidebar();
            buttonOpenRightSidebar->setVisible(false);
        });
    }
}

void WorkspaceToolBar::setWorkspaceName(const QString &name) {
    labelWorkspaceName->setText(
            name.isEmpty()
            ? ""
            : ("Workspace: <b>" + name + "</b>")
    );
}

void WorkspaceToolBar::showButtonOpenRightSidebar() {
    buttonOpenRightSidebar->setVisible(true);
}

void WorkspaceToolBar::setUpWorkspaceSettingsMenu() {
    workspaceSettingsMenu = new QMenu(this);

    {
        workspaceSettingsMenu->addAction("Card Colors...", this, [this]() {
            emit openCardColorsDialog();
        });
    }
}

void WorkspaceToolBar::setUpConnections() {
    connect(workspaceSettingsMenu, &QMenu::aboutToHide, this, [this]() {
        buttonWorkspaceSettings->update();
                // without this, the button's appearence stay in hover state
    });
}
