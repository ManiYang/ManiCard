#include <QLineEdit>
#include <QVBoxLayout>
#include "custom_list_widget.h"

CustomListWidget::CustomListWidget(QWidget *parent)
        : QFrame(parent) {
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    listWidget = new ListWidgetTweak;
    layout->addWidget(listWidget);

    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setDragDropMode(QListWidget::InternalMove);
    listWidget->setDefaultDropAction(Qt::MoveAction);
    listWidget->setFocusPolicy(Qt::NoFocus);
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    setUpConnections();
}

void CustomListWidget::addItem(const int itemId, const QString &text) {
    auto *foundItem = findItemById(itemId).second;
    if (foundItem != nullptr) {
        foundItem->setText(text);
        return;
    }

    auto *item = new QListWidgetItem;
    item->setText(text);
    item->setData(Qt::UserRole, itemId);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    listWidget->addItem(item);
}

void CustomListWidget::setItemText(const int itemId, const QString &text) {
    auto *item = findItemById(itemId).second;
    if (item != nullptr)
        item->setText(text);
}

void CustomListWidget::startEditItem(const int itemId) {
    auto *item = findItemById(itemId).second;
    if (item == nullptr)
        return;
    listWidget->editItem(item);
    startedEditItemId = itemId;
}

void CustomListWidget::ensureItemVisible(const int itemId) {
    auto *item = findItemById(itemId).second;
    if (item == nullptr)
        return;
    listWidget->scrollToItem(item);
}

void CustomListWidget::removeItem(const int itemId) {
    const int row = findItemById(itemId).first;
    if (row != -1) {
        auto *item = listWidget->takeItem(row);
        if (highlightedItem == item)
            highlightedItem = nullptr;
        delete item;
    }
}

void CustomListWidget::clear() {
    listWidget->clear();
    highlightedItem = nullptr;
}

void CustomListWidget::setSelectedItemId(const int itemId) {
    QListWidgetItem *itemToHighlight = findItemById(itemId).second; // can be nullptr
    setHighlightedItem(itemToHighlight);
}

void CustomListWidget::onItemContextMenuClosed() {
    listWidget->setCurrentRow(-1);
}

void CustomListWidget::setHighlightColor(const QColor &color) {
    highlightColor = color;
    setHighlightedItem(highlightedItem);
}

int CustomListWidget::count() const {
    return listWidget->count();
}

QVector<int> CustomListWidget::getItems() const {
    return allItemIds();
}

int CustomListWidget::selectedItemId() const {
    return highlightedItemId();
}

QString CustomListWidget::textOfItem(const int itemId) const {
    auto *item = findItemById(itemId).second;
    if (item != nullptr)
        return item->text();
    else
        return "";
}

void CustomListWidget::setUpConnections() {
    connect(listWidget, &ListWidgetTweak::gotDropEvent, this, [this]() {
        const QVector<int> itemIds = allItemIds();
        emit itemsOrderChanged(itemIds);

        listWidget->setCurrentRow(-1);
    });

    connect(listWidget, &ListWidgetTweak::itemClicked, this, [this](QListWidgetItem *item) {
        if (item != highlightedItem) {
            const int previousItemId = highlightedItemId();

            setHighlightedItem(item);

            bool ok;
            const int itemId = item->data(Qt::UserRole).toInt(&ok);
            Q_ASSERT(ok);
            emit itemSelected(itemId, previousItemId);
        }

        listWidget->setCurrentRow(-1);
    });

    connect(listWidget, &ListWidgetTweak::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        auto *item = listWidget->itemAt(pos);
        if (item != nullptr) {
            bool ok;
            const int itemId = item->data(Qt::UserRole).toInt(&ok);
            Q_ASSERT(ok);

            const auto screenPos = listWidget->viewport()->mapToGlobal(pos);
            emit itemContextMenuRequested(itemId, screenPos);
        }
    });

    connect(listWidget->itemDelegate(), &QAbstractItemDelegate::commitData,
            this, [this](QWidget *editor) {
        auto *lineEdit = qobject_cast<QLineEdit *>(editor);
        Q_ASSERT(lineEdit != nullptr);
        const QString text = lineEdit->text().trimmed();

        emit itemTextEdited(startedEditItemId, text);
        startedEditItemId = -1;
    });
}

void CustomListWidget::setHighlightedItem(QListWidgetItem *item) {
    if (highlightedItem != nullptr)
        highlightedItem->setBackground(Qt::transparent);

    if (item != nullptr)
        item->setBackground(highlightColor);

    highlightedItem = item;
}

std::pair<int, QListWidgetItem *> CustomListWidget::findItemById(const int itemId) const {
    for (int r = 0; r < listWidget->count(); ++r) {
        auto *item = listWidget->item(r);
        if (getItemId(listWidget->item(r)) == itemId)
            return {r, item};
    }
    return {-1, nullptr};
}

QVector<int> CustomListWidget::allItemIds() const {
    QVector<int> itemIds;
    itemIds.reserve(listWidget->count());
    for (int r = 0; r < listWidget->count(); ++r)
        itemIds << getItemId(listWidget->item(r));
    return itemIds;
}

int CustomListWidget::highlightedItemId() const {
    if (highlightedItem == nullptr)
        return -1;
    return getItemId(highlightedItem);
}

int CustomListWidget::getItemId(QListWidgetItem *item) {
    Q_ASSERT(item != nullptr);
    bool ok;
    const int itemId = item->data(Qt::UserRole).toInt(&ok);
    Q_ASSERT(ok);
    return itemId;
}

//====

ListWidgetTweak::ListWidgetTweak(QWidget *parent)
    : QListWidget(parent) {
}

void ListWidgetTweak::dropEvent(QDropEvent *event) {
    QListWidget::dropEvent(event);
    emit gotDropEvent();
}
