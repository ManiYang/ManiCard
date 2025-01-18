#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "services.h"
#include "utilities/maps_util.h"
#include "utilities/lists_vectors_util.h"
#include "widgets/app_style_sheet.h"
#include "widgets/components/custom_list_widget.h"
#include "workspaces_list.h"

WorkspacesList::WorkspacesList(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpBoardContextMenu();
    setUpConnections();
}

void WorkspacesList::resetWorkspaces(
        const QHash<int, QString> &workspaceIdToName, const QVector<int> &workspacesOrdering) {
    const int selectedWorkspaceId = listWidget->selectedItemId(); // can be -1
    listWidget->clear();

    //
    const QVector<int> sortedWorkspaceIds
            = sortByOrdering(keySet(workspaceIdToName), workspacesOrdering, false);
    for (const int boardId: sortedWorkspaceIds)
        listWidget->addItem(boardId, workspaceIdToName.value(boardId));

    listWidget->setSelectedItemId(selectedWorkspaceId);
}

void WorkspacesList::addWorkspace(const int workspaceId, const QString &name) {
    Q_ASSERT(!listWidget->getItems().contains(workspaceId));
    listWidget->addItem(workspaceId, name);
}

void WorkspacesList::setWorkspaceName(const int workspaceId, const QString &name) {
    listWidget->setItemText(workspaceId, name);
}

void WorkspacesList::startEditWorkspaceName(const int workspaceId) {
    listWidget->ensureItemVisible(workspaceId);
    listWidget->startEditItem(workspaceId);
}

void WorkspacesList::setSelectedWorkspaceId(const int workspaceId) {
    listWidget->setSelectedItemId(workspaceId);
}

void WorkspacesList::removeWorkspace(const int workspaceId) {
    listWidget->removeItem(workspaceId);
}

int WorkspacesList::count() const {
    return listWidget->count();
}

QVector<int> WorkspacesList::getWorkspaceIds() const {
    return listWidget->getItems();
}

QString WorkspacesList::workspaceName(const int workspaceId) const {
    return listWidget->textOfItem(workspaceId);
}

int WorkspacesList::selectedWorkspaceId() const {
    return listWidget->selectedItemId(); // can be -1
}

void WorkspacesList::setUpWidgets() {
    auto *rootVLayout = new QVBoxLayout;
    setLayout(rootVLayout);
    rootVLayout->setContentsMargins(0, 0, 0, 0);
    {
        auto *topHLayout = new QHBoxLayout;
        rootVLayout->addLayout(topHLayout);
        topHLayout->setContentsMargins(14, 0, 0, 0);
        {
            buttonNewWorkspace = new QPushButton(QIcon(":/icons/add_black_24"), "New Workspace");
            topHLayout->addWidget(buttonNewWorkspace);

            topHLayout->addStretch();
        }

        listWidget = new CustomListWidget;
        rootVLayout->addWidget(listWidget);
        {
            listWidget->setFrameShape(QFrame::NoFrame);

            const bool isDarkTheme = Services::instance()->getAppDataReadonly()->getIsDarkTheme();
            listWidget->setHighlightColor(getListWidgetHighlightedItemColor(isDarkTheme));
        }
    }

    //
    setStyleClasses(this, {StyleClass::highContrastBackground});

    listWidget->setStyleSheet(
            "QListWidget {"
            "  font-size: 11pt;"
            "}");

    setStyleClasses(buttonNewWorkspace, {StyleClass::flatPushButton});
}

void WorkspacesList::setUpConnections() {
    connect(listWidget, &CustomListWidget::itemSelected,
            this, [this](int workspaceId, int previousWorkspaceId) {
        emit workspaceSelected(workspaceId, previousWorkspaceId);
    });

    connect(listWidget, &CustomListWidget::itemsOrderChanged,
            this, [this](QVector<int> workspaceIds) {
        emit workspacesOrderChanged(workspaceIds);
    });

    connect(listWidget, &CustomListWidget::itemContextMenuRequested,
            this, [this](int itemId, QPoint screenPos) {
        workspaceIdOnContextMenuRequest = itemId;
        workspaceContextMenu->popup(screenPos);
    });

    connect(listWidget, &CustomListWidget::itemTextEdited,
            this, [this](int itemId, QString text) {
        text = text.trimmed();
        if (text.isEmpty())
            text = "untitled";
        emit userRenamedWorkspace(itemId, text);
    });

    //
    connect(workspaceContextMenu, &QMenu::aboutToHide, this, [this]() {
        listWidget->onItemContextMenuClosed();
    });

    //
    connect(buttonNewWorkspace, &QPushButton::clicked, this, [this]() {
        emit userToCreateNewWorkspace();
    });

    //
    connect(Services::instance()->getAppDataReadonly(), &AppDataReadonly::isDarkThemeUpdated,
            this, [this](bool const isDarkTheme) {
        listWidget->setHighlightColor(getListWidgetHighlightedItemColor(isDarkTheme));
    });
}

void WorkspacesList::setUpBoardContextMenu() {
    workspaceContextMenu = new QMenu(this);
    {
        auto *action = workspaceContextMenu->addAction(
                QIcon(":/icons/edit_square_black_24"), "Rename");
        connect(action, &QAction::triggered, this, [this]() {
            listWidget->startEditItem(workspaceIdOnContextMenuRequest);
        });
    }
    {
        auto *action = workspaceContextMenu->addAction(
                QIcon(":/icons/delete_black_24"), "Delete");
        connect(action, &QAction::triggered, this, [this]() {
            emit userToRemoveWorkspace(workspaceIdOnContextMenuRequest);
        });
    }
}

QColor WorkspacesList::getListWidgetHighlightedItemColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(72, 72, 72) : QColor(220, 220, 220);
}
