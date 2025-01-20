#include <QVBoxLayout>
#include "app_data_readonly.h"
#include "services.h"
#include "utilities/maps_util.h"
#include "utilities/lists_vectors_util.h"
#include "widgets/app_style_sheet.h"
#include "widgets/components/custom_list_widget.h"
#include "widgets/icons.h"
#include "workspaces_list.h"

WorkspacesList::WorkspacesList(QWidget *parent)
        : QFrame(parent) {
    setUpWidgets();
    setUpButtonsWithIcons();
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
            buttonNewWorkspace = new QPushButton("New Workspace");
            buttonToIcon.insert(buttonNewWorkspace, Icon::Add);
            topHLayout->addWidget(buttonNewWorkspace);

            topHLayout->addStretch();
        }

        listWidget = new CustomListWidget;
        rootVLayout->addWidget(listWidget);
        {
            listWidget->setFrameShape(QFrame::NoFrame);
            listWidget->setSpacing(2);

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

    setStyleClasses(
            buttonNewWorkspace, {StyleClass::flatPushButton, StyleClass::mediumContrastTextColor});
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
        workspaceContextMenu.workspaceIdOnContextMenuRequest = itemId;
        workspaceContextMenu.setActionIcons();
        workspaceContextMenu.actionDelete->setEnabled(listWidget->selectedItemId() == itemId);
        workspaceContextMenu.menu->popup(screenPos);
    });

    connect(listWidget, &CustomListWidget::itemTextEdited,
            this, [this](int itemId, QString text) {
        text = text.trimmed();
        if (text.isEmpty())
            text = "untitled";
        emit userRenamedWorkspace(itemId, text);
    });

    //
    connect(workspaceContextMenu.menu, &QMenu::aboutToHide, this, [this]() {
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

void WorkspacesList::setUpButtonsWithIcons() {
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

QColor WorkspacesList::getListWidgetHighlightedItemColor(const bool isDarkTheme) {
    return isDarkTheme ? QColor(54, 63, 74) : QColor(220, 220, 220);
}

//======

WorkspacesList::ContextMenu::ContextMenu(WorkspacesList *workspacesList_)
        : workspacesList(workspacesList_) {
    menu = new QMenu(workspacesList);
    {
        auto *action = menu->addAction("Rename");
        actionToIcon.insert(action, Icon::EditSquare);
        connect(action, &QAction::triggered, workspacesList, [this]() {
            workspacesList->listWidget->startEditItem(workspaceIdOnContextMenuRequest);
        });
    }
    {
        actionDelete = menu->addAction("Delete");
        actionToIcon.insert(actionDelete, Icon::Delete);
        connect(actionDelete, &QAction::triggered, workspacesList, [this]() {
            emit workspacesList->userToRemoveWorkspace(workspaceIdOnContextMenuRequest);
        });
    }
}

void WorkspacesList::ContextMenu::setActionIcons() {
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = actionToIcon.constBegin(); it != actionToIcon.constEnd(); ++it)
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
}
