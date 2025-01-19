#ifndef CUSTOMLISTWIDGET_H
#define CUSTOMLISTWIDGET_H

#include <QListWidget>

class ListWidgetTweak;

//!
//! Wraps a QListWidget. Supports the following operations:
//! - Select an item by clicking on it.
//! - Drag-and-drop an item to move it (reordering the items in the list) without selecting it.
//! - Right-click on an item to open context menu.
//!
//! Items are identified by item IDs.
//!
class CustomListWidget : public QFrame
{
    Q_OBJECT
public:
    explicit CustomListWidget(QWidget *parent = nullptr);

    //!
    //! If \e itemId already exists, only update the text.
    //!
    void addItem(const int itemId, const QString &text);

    void setItemText(const int itemId, const QString &text);

    //!
    //! Lets user edit the item.
    //!
    void startEditItem(const int itemId);

    void ensureItemVisible(const int itemId);

    void removeItem(const int itemId);

    void clear();

    //!
    //! \param itemId: if not found, will select none
    //!
    void setSelectedItemId(const int itemId);

    void onItemContextMenuClosed();

    void setHighlightColor(const QColor &color);

    void setSpacing(const int spacing);

    //

    int count() const;
    QVector<int> getItems() const;

    //!
    //! \return -1 if no item is selected
    //!
    int selectedItemId() const;

    //!
    //! \return "" if not found
    //!
    QString textOfItem(const int itemId) const;

signals:
    void itemSelected(int itemId, int previousItemId);
    void itemsOrderChanged(QVector<int> itemIds);
    void itemContextMenuRequested(int itemId, QPoint screenPos);
    void itemTextEdited(int itemId, QString text);

private:
    ListWidgetTweak *listWidget;
    QListWidgetItem *highlightedItem {nullptr};
    int startedEditItemId {-1};

    QColor highlightColor {220, 220, 220};

    //
    void setUpConnections();

    //!
    //! \param item: can be nullptr
    //!
    void setHighlightedItem(QListWidgetItem *item);

    std::pair<int, QListWidgetItem *> findItemById(const int itemId) const;
    QVector<int> allItemIds() const;
    int highlightedItemId() const; // returns -1 if none highlighted
    static int getItemId(QListWidgetItem *item); // `item` cannot be nullptr
};


class ListWidgetTweak : public QListWidget
{
    Q_OBJECT
public:
    explicit ListWidgetTweak(QWidget *parent = nullptr);

signals:
    void gotDropEvent();

protected:
    void dropEvent(QDropEvent *event) override;
};


#endif // CUSTOMLISTWIDGET_H
