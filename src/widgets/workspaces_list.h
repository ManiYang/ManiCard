#ifndef WORKSPACESLIST_H
#define WORKSPACESLIST_H

#include <QFrame>
#include <QMenu>
#include <QPushButton>
#include "widgets/icons.h"

class CustomListWidget;

class WorkspacesList : public QFrame
{
    Q_OBJECT
public:
    explicit WorkspacesList(QWidget *parent = nullptr);

    //!
    //! Originally selected workspace will be kept selected, unless it is not found in
    //! \e workspaceIdToName.
    //!
    void resetWorkspaces(
            const QHash<int, QString> &workspaceIdToName, const QVector<int> &workspacesOrdering);

    //!
    //! The workspace \a workspaceId must not already exist in the list.
    //!
    void addWorkspace(const int workspaceId, const QString &name);

    void setWorkspaceName(const int workspaceId, const QString &name);
    void startEditWorkspaceName(const int workspaceId);
    void setSelectedWorkspaceId(const int workspaceId);
    void removeWorkspace(const int workspaceId);

    //

    int count() const;
    QVector<int> getWorkspaceIds() const;

    //!
    //! \return "" if not found
    //!
    QString workspaceName(const int workspaceId) const;

    //!
    //! \return -1 if no board is selected
    //!
    int selectedWorkspaceId() const;

signals:
    void workspaceSelected(int newWorkspaceId, int previousWorkspaceId); // `previousWorkspaceId` can be -1
    void workspacesOrderChanged(QVector<int> workspaceIds);
    void userToCreateNewWorkspace();
    void userRenamedWorkspace(int workspaceId, QString name);
    void userToRemoveWorkspace(int workspaceId);

private:
    QPushButton *buttonNewWorkspace;
    CustomListWidget *listWidget;

    struct ContextMenu
    {
        explicit ContextMenu(WorkspacesList *workspacesList);
        QMenu *menu;
//        QAction *actionRename;
        QAction *actionDelete;
        int workspaceIdOnContextMenuRequest {-1};

        void setActionIcons();
    private:
        WorkspacesList *workspacesList;
        QHash<QAction *, Icon> actionToIcon;
    };
    ContextMenu workspaceContextMenu {this};

    QHash<QAbstractButton *, Icon> buttonToIcon;

    //
    void setUpWidgets();
    void setUpConnections();
    void setUpButtonsWithIcons();

    QColor getListWidgetHighlightedItemColor(const bool isDarkTheme);
};

#endif // WORKSPACESLIST_H
