#include <QHBoxLayout>
#include <QVBoxLayout>
#include "boards_list.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"
#include "widgets/components/custom_list_widget.h"

BoardsList::BoardsList(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpBoardContextMenu();
    setUpConnections();
}

void BoardsList::resetBoards(
        const QHash<int, QString> &boardIdToName, const QVector<int> &boardsOrdering) {
    const int selectedBoardId = listWidget->selectedItemId(); // can be -1
    listWidget->clear();

    //
    const QVector<int> sortedBoardIds
            = sortByOrdering(keySet(boardIdToName), boardsOrdering, false);
    for (const int boardId: sortedBoardIds)
        listWidget->addItem(boardId, boardIdToName.value(boardId));

    listWidget->setSelectedItemId(selectedBoardId);
}

void BoardsList::addBoard(const int boardId, const QString &name) {
    Q_ASSERT(!listWidget->getItems().contains(boardId));
    listWidget->addItem(boardId, name);
}

void BoardsList::setBoardName(const int boardId, const QString &name) {
    listWidget->setItemText(boardId, name);
}

void BoardsList::startEditBoardName(const int boardId) {
    listWidget->ensureItemVisible(boardId);
    listWidget->startEditItem(boardId);
}

void BoardsList::setSelectedBoardId(const int boardId) {
    listWidget->setSelectedItemId(boardId);
}

void BoardsList::removeBoard(const int boardId) {
    listWidget->removeItem(boardId);
}

QVector<int> BoardsList::getBoardsOrder() const {
    return listWidget->getItems();
}

QString BoardsList::boardName(const int boardId) const {
    return listWidget->textOfItem(boardId);
}

int BoardsList::selectedBoardId() const {
    return listWidget->selectedItemId(); // can be -1
}

void BoardsList::setUpWidgets() {
    auto *rootVLayout = new QVBoxLayout;
    setLayout(rootVLayout);
    rootVLayout->setContentsMargins(0, 0, 0, 0);
    {
        auto *topHLayout = new QHBoxLayout;
        rootVLayout->addLayout(topHLayout);
        topHLayout->setContentsMargins(14, 0, 0, 0);
        {
            buttonNewBoard = new QPushButton(QIcon(":/icons/add_black_24"), "New Board");
            topHLayout->addWidget(buttonNewBoard);

            topHLayout->addStretch();
        }

        listWidget = new CustomListWidget;
        rootVLayout->addWidget(listWidget);
        listWidget->setFrameShape(QFrame::NoFrame);
    }

    //
    listWidget->setStyleSheet(
            "QListWidget {"
            "  font-size: 11pt;"
            "  background: transparent;"
            "}");

    buttonNewBoard->setStyleSheet(
            "QPushButton {"
            "  color: #606060;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 2px 4px 2px 2px;"
            "  background: transparent;"
            "}"
            "QPushButton:hover {"
            "  background: #e0e0e0;"
            "}");
}

void BoardsList::setUpConnections() {
    connect(listWidget, &CustomListWidget::itemSelected,
            this, [this](int boardId, int previousBoardId) {
        emit boardSelected(boardId, previousBoardId);
    });

    connect(listWidget, &CustomListWidget::itemsOrderChanged,
            this, [this](QVector<int> boardIds) {
        emit boardsOrderChanged(boardIds);
    });

    connect(listWidget, &CustomListWidget::itemContextMenuRequested,
            this, [this](int itemId, QPoint screenPos) {
        boardIdOnContextMenuRequest = itemId;
        boardContextMenu->popup(screenPos);
    });

    connect(listWidget, &CustomListWidget::itemTextEdited,
            this, [this](int itemId, QString text) {
        text = text.trimmed();
        if (text.isEmpty())
            text = "untitled";
        emit userRenamedBoard(itemId, text);
    });

    //
    connect(boardContextMenu, &QMenu::aboutToHide, this, [this]() {
        listWidget->onItemContextMenuClosed();
    });

    //
    connect(buttonNewBoard, &QPushButton::clicked, this, [this]() {
        emit userToCreateNewBoard();
    });
}

void BoardsList::setUpBoardContextMenu() {
    boardContextMenu = new QMenu(this);
    {
        auto *action = boardContextMenu->addAction(
                QIcon(":/icons/edit_square_black_24"), "Rename");
        connect(action, &QAction::triggered, this, [this]() {
            listWidget->startEditItem(boardIdOnContextMenuRequest);
        });
    }
    {
        auto *action = boardContextMenu->addAction(
                QIcon(":/icons/delete_black_24"), "Delete");
        connect(action, &QAction::triggered, this, [this]() {
            emit userToRemoveBoard(boardIdOnContextMenuRequest);
        });
    }
}
