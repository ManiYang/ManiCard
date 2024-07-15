#ifndef BOARDSLIST_H
#define BOARDSLIST_H

#include <QFrame>
#include <QMenu>
#include <QPushButton>

class CustomListWidget;

class BoardsList : public QFrame
{
    Q_OBJECT
public:
    explicit BoardsList(QWidget *parent = nullptr);

    //!
    //! Originally selected board will be kept selected, unless it is not found in
    //! \e boardIdToName.
    //!
    void resetBoards(
            const QHash<int, QString> &boardIdToName, const QVector<int> &boardsOrdering);

    //!
    //! The board \a boardId must not already exist in the list.
    //!
    void addBoard(const int boardId, const QString &name);

    void setBoardName(const int boardId, const QString &name);
    void startEditBoardName(const int boardId);
    void setSelectedBoardId(const int boardId);
    void removeBoard(const int boardId);

    //

    QVector<int> getBoardsOrder() const;

    //!
    //! \return "" if not found
    //!
    QString boardName(const int boardId) const;

    //!
    //! \return -1 if no board is selected
    //!
    int selectedBoardId() const;

signals:
    void boardSelected(int newBoardId, int previousBoardId); // `previousBoardId` can be -1
    void boardsOrderChanged(QVector<int> boardIds);
    void userToCreateNewBoard();
    void userRenamedBoard(int boardId, QString name);
    void userToRemoveBoard(int boardId);

private:
    void setUpWidgets();
    void setUpConnections();
    void setUpBoardContextMenu();

    QPushButton *buttonNewBoard;
    CustomListWidget *listWidget;

    QMenu *boardContextMenu;
    int boardIdOnContextMenuRequest {-1};
};

#endif // BOARDSLIST_H
