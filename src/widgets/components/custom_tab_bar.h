#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QFrame>
#include <QString>
#include <QTabBar>

class CustomTabBar : public QFrame
{
    Q_OBJECT
public:
    explicit CustomTabBar(QWidget *parent = nullptr);

    //!
    //! Also sets current tab to the newly added tab.
    //! \param itemId: cannot be -1
    //! \param name
    //!
    void addTab(const int itemId, const QString &name);

    void setCurrentItemId(const int itemId);

    void renameItem(const int itemId, const QString &newName);

    void removeItem(const int itemId);

    void removeAllTabs();

    //

    int count() const;

    //!
    //! \return (-1, "") if not found
    //!
    std::pair<int, QString> getCurrentItemIdAndName() const;

    //!
    //! \return "" if not found
    //!
    QString getItemNameById(const int itemId) const;

    //!
    //! \return -1 if not found
    //!
    int getItemIdByTabIndex(const int tabIndex) const;

    //!
    //! \return item IDs in the tab order
    //!
    QVector<int> getAllItemIds() const;

signals:
    void tabSelectedByUser(const int itemId);

    //!
    //! \param itemIdUnderMouseCursor: can be -1
    //! \param globalPos
    //!
    void contextMenuRequested(int itemIdUnderMouseCursor, QPoint globalPos);

    void tabsReorderedByUser(const QVector<int> &itemIdsOrdering);

private:
    QTabBar *tabBar;
    int currentItemId {-1};
    bool handleSignalsAsUserOperation {true};

    // tools

    //!
    //! \return -1 if not found
    //!
    int getItemId(const int tabIndex) const;

    //!
    //! \return -1 if not found
    //!
    int getTabIndexByItemId(const int itemId) const;
};

#endif // CUSTOMTABBAR_H
