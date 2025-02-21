#include <QDebug>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include "app_data.h"
#include "app_data_readonly.h"
#include "services.h"
#include "utilities/async_routine.h"
#include "utilities/filenames_util.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "utilities/message_box.h"
#include "utilities/periodic_checker.h"
#include "widgets/app_style_sheet.h"
#include "widgets/board_view.h"
#include "widgets/components/custom_tab_bar.h"
#include "widgets/dialogs/dialog_workspace_card_colors.h"
#include "workspace_frame.h"

using ContinuationContext = AsyncRoutineWithErrorFlag::ContinuationContext;

WorkspaceFrame::WorkspaceFrame(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpConnections();
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
        // close `boardView`
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
        // clear `boardsTabBar`
        ContinuationContext context(routine);

        boardsTabBar->removeAllTabs();

        // clear variables
        workspaceId = -1;
        workspaceName = "";
        workspaceToolBar->setWorkspaceName("");
    }, this);

    routine->addStep([this, routine, workspaceIdToLoad]() {
        // get workspace data
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
        // get board names
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
        // populate `boardsTabBar` and determine `routine->boardIdToOpen`
        ContinuationContext context(routine);

        const QVector<int> sortedBoardIds = sortByOrdering(
                keySet(routine->boardIdToName), routine->workspaceData.boardsOrdering, false);
        for (const int boardId: sortedBoardIds) {
            const QString boardName = routine->boardIdToName.value(boardId);
            boardsTabBar->addTab(boardId, boardName);
        }

        //
        if (sortedBoardIds.isEmpty()) {
            routine->boardIdToOpen = -1;
        }
        else {
            routine->boardIdToOpen = sortedBoardIds.contains(routine->workspaceData.lastOpenedBoardId)
                    ? routine->workspaceData.lastOpenedBoardId
                    : sortedBoardIds.at(0);
            boardsTabBar->setCurrentItemId(routine->boardIdToOpen);
        }
    }, this);

    routine->addStep([this, routine]() {
        // open board
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
            cardLabelToColorMapping = routine->workspaceData.cardLabelToColorMapping;
            cardPropertiesToShow = routine->workspaceData.cardPropertiesToShow;

            // set `boardView`'s card color mapping to the same as this->cardLabelToColorMapping
            boardView->setColorsAssociatedWithLabels(
                    cardLabelToColorMapping.cardLabelsAndAssociatedColors,
                    cardLabelToColorMapping.defaultNodeRectColor);
            boardView->cardPropertiesToShowSettingOnWorkspaceUpdated(
                    routine->workspaceData.cardPropertiesToShow);
        }

        callback(!routine->errorFlag, routine->highlightedCardIdChanged);
    }, this);

    routine->start();
}

void WorkspaceFrame::changeWorkspaceName(const QString newName) {
    workspaceName = newName;
    workspaceToolBar->setWorkspaceName(newName);
}

void WorkspaceFrame::showButtonRightSidebar() {
    workspaceToolBar->showButtonOpenRightSidebar();
}

void WorkspaceFrame::applyZoomAction(const ZoomAction zoomAction) {
    if (boardView->isVisible())
        boardView->applyZoomAction(zoomAction);
}

void WorkspaceFrame::toggleCardPreview() {
    if (boardView->isVisible())
        boardView->toggleCardPreview();
}

void WorkspaceFrame::prepareToClose() {
    boardView->prepareToClose();
}

int WorkspaceFrame::getWorkspaceId() const {
    return workspaceId;
}

int WorkspaceFrame::getCurrentBoardId() {
    return boardView->getBoardId();
}

QVector<int> WorkspaceFrame::getAllBoardIds() const {
    return boardsTabBar->getAllItemIds();
}

QPointF WorkspaceFrame::getBoardViewTopLeftPos() const {
    return boardView->getViewTopLeftPos();
}

double WorkspaceFrame::getBoardViewZoomRatio() const {
    return boardView->getZoomRatio();
}

bool WorkspaceFrame::canClose() const {
    return boardView->canClose();
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
        boardsTabBar = new CustomTabBar;
        layout->addWidget(boardsTabBar);

        //
        boardView = new BoardView;
        layout->addWidget(boardView);
        boardView->setVisible(false);

        //
        noBoardSign = new NoBoardSign;
        layout->addWidget(noBoardSign);
        noBoardSign->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

void WorkspaceFrame::setUpConnections() {
    // `workspaceToolBar`
    connect(workspaceToolBar, &WorkspaceToolBar::userToAddNewBoard, this, [this]() {
        onUserToAddBoard();
    });

    connect(workspaceToolBar, &WorkspaceToolBar::openRightSidebar, this, [this]() {
        emit openRightSidebar();
    });

    connect(workspaceToolBar, &WorkspaceToolBar::openCardColorsDialog, this, [this]() {
        onUserToSetCardColors();
    });

    // `boardsTabBar`
    connect(boardsTabBar, &CustomTabBar::contextMenuRequested,
            this, [this](int itemIdUnderMouseCursor, QPoint globalPos) {
        const int currentBoardId = boardsTabBar->getCurrentItemIdAndName().first;
        const bool isOnCurrentTab = (currentBoardId != -1)
                ? (itemIdUnderMouseCursor == currentBoardId)
                : false;

        if (isOnCurrentTab) {
            boardTabContextMenu.boardTabContextMenuTargetBoardId = currentBoardId;
            boardTabContextMenu.setActionIcons();
            boardTabContextMenu.menu->popup(globalPos);
        }
    });

    connect(boardsTabBar, &CustomTabBar::tabSelectedByUser, this, [this](const int boardId) {
        onUserSelectedBoard(boardId);
    });

    connect(boardsTabBar, &CustomTabBar::tabsReorderedByUser,
            this, [this](const QVector<int> &/*boardIdsOrdering*/) {
        saveBoardsOrdering();
    });

    // `boardView`
    connect(boardView, &BoardView::workspaceCardLabelToColorMappingUpdatedViaSettingBox,
            this,
            [this](const int workspaceId, const CardLabelToColorMapping &cardLabelToColorMapping) {
        if (this->workspaceId != workspaceId) {
            qWarning().noquote()
                    << "SettingBox edits setting of a workspace other than the current one";
            return;
        }

        onCardLabelToColorMappingUpdated(cardLabelToColorMapping);
    });

    connect(boardView, &BoardView::workspaceCardPropertiesToShowUpdatedViaSettingBox,
            this,
            [this](const int workspaceId, const CardPropertiesToShow &cardPropertiesToShow) {
        if (this->workspaceId != workspaceId) {
            qWarning().noquote()
                    << "SettingBox edits setting of a workspace other than the current one";
            return;
        }

        onCardPropertiesToShowUpdated(cardPropertiesToShow);
    });

    connect(boardView, &BoardView::hasWorkspaceSettingsPendingUpdateChanged,
            this, [this](bool hasWorkspaceSettingsPendingUpdate) {
        workspaceToolBar->setWorkspaceSettingsMenuEnabled(!hasWorkspaceSettingsPendingUpdate);
    });

    // `noBoardSign`
    connect(noBoardSign, &NoBoardSign::userToAddBoard, this, [this]() {
        onUserToAddBoard();
    });
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
        boardsTabBar->setEnabled(false);

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
        saveTopLeftPosAndZoomRatioOfCurrentBoard();

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
                        Services::instance()->getAppData()
                                ->setSingleHighlightedCardId(EventSource(this), -1);
                    }
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        // add to `boardsTabBar`
        ContinuationContext context(routine);
        boardsTabBar->addTab(routine->newBoardId, routine->newBoardData.name);
        saveBoardsOrdering();
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);

        boardsTabBar->setEnabled(true);
    }, this);

    routine->start();
}

void WorkspaceFrame::onUserToRenameBoard(const int boardId) {
    if (boardId == -1)
        return;

    // get new name from user
    QString newName;
    {
        const QString originalName = boardsTabBar->getItemNameById(boardId);

        bool ok;
        const QString newNameByUser = QInputDialog::getText(
                this, "Rename Board", "Enter new name:", QLineEdit::Normal, originalName, &ok);
        if (!ok)
            return;
        newName = !newNameByUser.isEmpty() ? newNameByUser : "untitled";
    }

    //
    boardsTabBar->renameItem(boardId, newName);

    //
    BoardNodePropertiesUpdate update;
    {
        update.name = newName;
    }
    Services::instance()->getAppData()->updateBoardNodeProperties(EventSource(this), boardId, update);
}

void WorkspaceFrame::onUserToRenameExportBoardToImage(const int boardId) {
    if (boardId != getCurrentBoardId()) {
        qWarning().noquote()
                << "exporting a board that is not currently opened is not implemented yet";
        return;
    }

    //
    const QString boardName = boardsTabBar->getItemNameById(boardId);
    const QString fileName
            = QString("%1__%2.png")
              .arg(makeValidFileName(workspaceName), makeValidFileName(boardName));
    const QImage image = boardView->renderAsImage();

    //
    const QString outputDir = Services::instance()->getAppDataReadonly()->getExportOutputDir();
    const QString filePath = QDir(outputDir).filePath(fileName);
    const bool ok = image.save(filePath);
    if (ok)
        QMessageBox::information(this, " ", "Successfully exported to " + filePath);
    else
        QMessageBox::warning(this, " ", "Failed to write image file " + filePath);
}

void WorkspaceFrame::onUserSelectedBoard(const int boardId) {
    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    //
    routine->addStep([this, routine]() {
        // prepare to close `boardView`
        boardsTabBar->setEnabled(false);

        saveTopLeftPosAndZoomRatioOfCurrentBoard();

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

    routine->addStep([this, routine, boardId]() {
        // load board
        boardView->loadBoard(
                boardId,
                // callback
                [this, routine, boardId](bool ok, bool highlightedCardIdChanged) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("Could not load board %1").arg(boardId);
                    }

                    if (highlightedCardIdChanged) {
                        Services::instance()->getAppData()
                                ->setSingleHighlightedCardId(EventSource(this), -1);
                    }
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);

        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);

        boardsTabBar->setEnabled(true);
    }, this);

    routine->start();
}

void WorkspaceFrame::onUserToRemoveBoard(const int boardIdToRemove) {
    if (boardIdToRemove == -1)
        return;

    // show confirmation message box
    const QString boardName = boardsTabBar->getItemNameById(boardIdToRemove);
    const auto r = QMessageBox::question(
            this, "Please confirm", QString("Remove the board \"%1\"?").arg(boardName));

    if (r != QMessageBox::Yes)
        return;

    // remove the board
    boardsTabBar->removeItem(boardIdToRemove);
    saveBoardsOrdering();
    Services::instance()->getAppData()->removeBoard(EventSource(this), boardIdToRemove);

    // select another board (if any)
    const int boardIdToLoad = (boardsTabBar->count() != 0)
            ? boardsTabBar->getItemIdByTabIndex(0)
            : -1;

    if (boardIdToLoad != -1)
        boardsTabBar->setCurrentItemId(boardIdToLoad);

    class AsyncRoutineWithVars : public AsyncRoutineWithErrorFlag
    {
    public:
        QString errorMsg;
    };
    auto *routine = new AsyncRoutineWithVars;

    routine->addStep([this, routine]() {
        // prepare to close `boardView`
        boardsTabBar->setEnabled(false);

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

    routine->addStep([this, routine, boardIdToLoad]() {
        // load board
        boardView->loadBoard(
                boardIdToLoad,
                // callback
                [this, routine, boardIdToLoad](bool ok, bool highlightedCardIdChanged) {
                    ContinuationContext context(routine);
                    if (!ok) {
                        context.setErrorFlag();
                        routine->errorMsg = QString("Could not load board %1").arg(boardIdToLoad);
                    }

                    if (highlightedCardIdChanged) {
                        Services::instance()->getAppData()
                                ->setSingleHighlightedCardId(EventSource(this), -1);
                    }
                }
        );
    }, this);

    routine->addStep([this, routine]() {
        // show no-board sign if workspace has no board
        ContinuationContext context(routine);
        if (boardsTabBar->count() == 0) {
            boardView->setVisible(false);
            noBoardSign->setVisible(true);
        }
    }, this);

    routine->addStep([this, routine]() {
        // final step
        ContinuationContext context(routine);
        if (routine->errorFlag && !routine->errorMsg.isEmpty())
            showWarningMessageBox(this, " ", routine->errorMsg);

        boardsTabBar->setEnabled(true);
    }, this);

    routine->start();
}

void WorkspaceFrame::onUserToSetCardColors() {
    if (workspaceId == -1)
        return;

    //
    auto *dialog = new DialogWorkspaceCardColors(
            workspaceName,
            boardView->getCardLabelsAndAssociatedColors(),
            boardView->getDefaultNodeRectColor(),
            this
    );

    connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
        dialog->deleteLater();

        if (result != QDialog::Accepted)
            return;

        CardLabelToColorMapping newSetting;
        {
            newSetting.defaultNodeRectColor = dialog->getDefaultColor();
            newSetting.cardLabelsAndAssociatedColors = dialog->getCardLabelsAndAssociatedColors();
        }
        onCardLabelToColorMappingUpdated(newSetting); // saves to AppData

        // inform `boardView` to update the corresponding SettingBox that is shown in it
        boardView->updateSettingBoxOnWorkspaceSetting(
                workspaceId, SettingCategory::CardLabelToColorMapping);
    });

    dialog->open();
}

void WorkspaceFrame::onCardLabelToColorMappingUpdated(
        const CardLabelToColorMapping &cardLabelToColorMapping_) {
    cardLabelToColorMapping = cardLabelToColorMapping_;

    // set `boardView`'s card color mapping to the same as this->cardLabelToColorMapping
    boardView->setColorsAssociatedWithLabels(
            cardLabelToColorMapping.cardLabelsAndAssociatedColors,
            cardLabelToColorMapping.defaultNodeRectColor);

    //
    WorkspaceNodePropertiesUpdate update;
    update.cardLabelToColorMapping = cardLabelToColorMapping_;

    Services::instance()->getAppData()->updateWorkspaceNodeProperties(
                EventSource(this), workspaceId, update);
}

void WorkspaceFrame::onCardPropertiesToShowUpdated(
        const CardPropertiesToShow &cardPropertiesToShow_) {
    Q_ASSERT(workspaceId != -1);

    cardPropertiesToShow = cardPropertiesToShow_;

    //
    boardView->cardPropertiesToShowSettingOnWorkspaceUpdated(cardPropertiesToShow);

    //
    WorkspaceNodePropertiesUpdate update;
    update.cardPropertiesToShow = cardPropertiesToShow_;

    Services::instance()->getAppData()->updateWorkspaceNodeProperties(
            EventSource(this), workspaceId, update);
}

void WorkspaceFrame::saveTopLeftPosAndZoomRatioOfCurrentBoard() {
    const int boardId = boardView->getBoardId();
    if (boardId == -1)
        return;

    BoardNodePropertiesUpdate propertiesUpdate;
    {
        propertiesUpdate.topLeftPos = boardView->getViewTopLeftPos();
        propertiesUpdate.zoomRatio = boardView->getZoomRatio();
    }
    Services::instance()->getAppData()->updateBoardNodeProperties(
                EventSource(this), boardId, propertiesUpdate);
}

void WorkspaceFrame::saveBoardsOrdering() {
    WorkspaceNodePropertiesUpdate update;
    update.boardsOrdering = boardsTabBar->getAllItemIds();

    Services::instance()->getAppData()->updateWorkspaceNodeProperties(
                EventSource(this), workspaceId, update);
}

//========

WorkspaceToolBar::WorkspaceToolBar(QWidget *parent)
        : SimpleToolBar(parent) {
    setUpWorkspaceSettingsMenu();

    //
    QPushButton *buttonNewBoard;

    {
        labelWorkspaceName = new QLabel;
        hLayout->addWidget(labelWorkspaceName);
    }
    hLayout->addStretch();
    {
        buttonNewBoard = new QPushButton("New Board");
        buttonToIcon.insert(buttonNewBoard, Icon::Add);
        hLayout->addWidget(buttonNewBoard);

        connect(buttonNewBoard, &QPushButton::clicked, this, [this]() {
            emit userToAddNewBoard();
        });
    }
    {
        buttonWorkspaceSettings = new QToolButton;
        hLayout->addWidget(buttonWorkspaceSettings);
        hLayout->setAlignment(buttonWorkspaceSettings, Qt::AlignVCenter);

        buttonToIcon.insert(buttonWorkspaceSettings, Icon::MoreVert);
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

        buttonToIcon.insert(buttonOpenRightSidebar, Icon::OpenRightPanel);
        buttonOpenRightSidebar->setIconSize({24, 24});
        buttonOpenRightSidebar->setToolTip("Open Right Side-Bar");

        connect(buttonOpenRightSidebar, &QToolButton::clicked, this, [this]() {
            emit openRightSidebar();
            buttonOpenRightSidebar->setVisible(false);
        });
    }

    // styles
    labelWorkspaceName->setStyleSheet(
            "QLabel {"
            "  font-size: 12pt;"
            "  margin-left: 4px;"
            "  background: transparent;"
            "}");

    setStyleClasses(
            buttonNewBoard, {StyleClass::flatPushButton, StyleClass::mediumContrastTextColor});

    setStyleClasses(buttonWorkspaceSettings, {StyleClass::flatToolButton});

    setStyleClasses(buttonOpenRightSidebar, {StyleClass::flatToolButton});

    //
    setUpConnections();
    setUpButtonsWithIcons();
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

void WorkspaceToolBar::setWorkspaceSettingsMenuEnabled(const bool enabled) {
    const auto menuActions = workspaceSettingsMenu->actions();
    for (QAction *action: menuActions)
        action->setEnabled(enabled);
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

void WorkspaceToolBar::setUpButtonsWithIcons() {
    // set the icons with current theme
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = buttonToIcon.constBegin(); it != buttonToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));

    // connect to "theme updated" signal
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](const bool isDarkTheme) {
        const auto theme = isDarkTheme ? Icons::Theme::Dark : Icons::Theme::Light;
        for (auto it = buttonToIcon.constBegin(); it != buttonToIcon.constEnd(); ++it)
            it.key()->setIcon(Icons::getIcon(it.value(), theme));
    });
}

//========

NoBoardSign::NoBoardSign(QWidget *parent)
        : QFrame(parent) {
    auto *vLayout = new QVBoxLayout;
    setLayout(vLayout);
    vLayout->setSpacing(12);

    vLayout->addStretch();
    {
        auto *label = new QLabel("This workspace has no board");
        vLayout->addWidget(label, 0, Qt::AlignHCenter);

        setStyleClasses(label, {StyleClass::mediumContrastTextColor});
        label->setStyleSheet(
                "QLabel {"
                "  font-size: 14pt;"
                "  font-weight: bold;"
                "}");
    }
    {
        auto *button = new QPushButton("Add Board");
        vLayout->addWidget(button, 0, Qt::AlignHCenter);

        connect(button, &QPushButton::clicked, this, [this]() {
            emit userToAddBoard();
        });

        setStyleClasses(button, {StyleClass::flatPushButton, StyleClass::mediumContrastTextColor});
        button->setStyleSheet(
                "QPushButton {"
                "  font-size: 12pt;"
                "  border: 1px solid #888888;"
                "  padding: 4px 12px"
                "}");
    }
    vLayout->addStretch();
}

//======

WorkspaceFrame::ContextMenu::ContextMenu(WorkspaceFrame *workspaceFrame_)
        : workspaceFrame(workspaceFrame_) {
    menu = new QMenu(workspaceFrame);
    {
        auto *action = menu->addAction("Rename Board...");
        actionToIcon.insert(action, Icon::EditSquare);
        connect(action, &QAction::triggered, workspaceFrame, [this]() {
            workspaceFrame->onUserToRenameBoard(boardTabContextMenuTargetBoardId);
        });
    }
    {
        auto *action = menu->addAction("Export to Image");
        actionToIcon.insert(action, Icon::FileSave);
        connect(action, &QAction::triggered, workspaceFrame, [this]() {
            workspaceFrame->onUserToRenameExportBoardToImage(boardTabContextMenuTargetBoardId);
        });
    }
    {
        auto *action = menu->addAction("Delete Board");
        actionToIcon.insert(action, Icon::Delete);
        connect(action, &QAction::triggered, workspaceFrame, [this]() {
            workspaceFrame->onUserToRemoveBoard(boardTabContextMenuTargetBoardId);
        });
    }
}

void WorkspaceFrame::ContextMenu::setActionIcons() {
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = actionToIcon.constBegin(); it != actionToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
}
