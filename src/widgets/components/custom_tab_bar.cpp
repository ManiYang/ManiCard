#include <QVariant>
#include <QVBoxLayout>
#include "custom_tab_bar.h"

CustomTabBar::CustomTabBar(QWidget *parent)
        : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);

    //
    auto *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    {
        tabBar = new QTabBar;
        layout->addWidget(tabBar, 0, Qt::AlignBottom);

        tabBar->setExpanding(false);
        tabBar->setMovable(true);
        tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    }

    //
    connect(tabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (handleSignalsAsUserOperation)  {
            const int itemId = getItemId(index);
            if (itemId != -1)
                emit tabSelectedByUser(itemId);
        }
    });

    connect(tabBar, &QTabBar::customContextMenuRequested, this, [this](const QPoint &pos) {
        const int tabIndexUnderMouseCursor = tabBar->tabAt(pos); // can be -1
        const int itemIdUnderMouseCursor = getItemId(tabIndexUnderMouseCursor); // can be -1
        emit contextMenuRequested(itemIdUnderMouseCursor, tabBar->mapToGlobal(pos));
    });
}

void CustomTabBar::addTab(const int itemId, const QString &name) {
    Q_ASSERT(itemId != -1);

    handleSignalsAsUserOperation = false;

    const int tabIndex = tabBar->addTab(name);
    tabBar->setTabData(tabIndex, QVariant(itemId));
    tabBar->setCurrentIndex(tabIndex);

    handleSignalsAsUserOperation = true;
}

void CustomTabBar::setCurrentItemId(const int itemId) {
    handleSignalsAsUserOperation = false;

    const int index = getTabIndexByItemId(itemId);
    if (index != -1)
        tabBar->setCurrentIndex(index);

    handleSignalsAsUserOperation = true;
}

void CustomTabBar::renameItem(const int itemId, const QString &newName) {
    handleSignalsAsUserOperation = false;

    const int index = getTabIndexByItemId(itemId);
    if (index != -1)
        tabBar->setTabText(index, newName);

    handleSignalsAsUserOperation = true;
}

void CustomTabBar::removeItem(const int itemId) {
    handleSignalsAsUserOperation = false;

    const int index = getTabIndexByItemId(itemId);
    if (index != -1)
        tabBar->removeTab(index);

    handleSignalsAsUserOperation = true;
}

void CustomTabBar::removeAllTabs() {
    handleSignalsAsUserOperation = false;

    while (tabBar->count() > 0)
        tabBar->removeTab(0);

    handleSignalsAsUserOperation = true;
}

int CustomTabBar::count() const {
    return tabBar->count();
}

std::pair<int, QString> CustomTabBar::getCurrentItemIdAndName() const {
    const int currentIndex = tabBar->currentIndex();
    if (currentIndex == -1)
        return {-1, ""};

    const int itemId = getItemId(currentIndex);
    Q_ASSERT(itemId != -1);
    return {itemId, tabBar->tabText(currentIndex)};
}

QString CustomTabBar::getItemNameById(const int itemId) const {
    const int index = getTabIndexByItemId(itemId); // can be -1
    if (index == -1)
        return "";
    return tabBar->tabText(index);
}

int CustomTabBar::getItemIdByTabIndex(const int tabIndex) const {
    return getItemId(tabIndex);
}

int CustomTabBar::getItemId(const int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= tabBar->count())
        return -1;

    bool ok;
    const int itemId = tabBar->tabData(tabIndex).toInt(&ok);
    Q_ASSERT(ok);
    return itemId;
}

int CustomTabBar::getTabIndexByItemId(const int itemId) const {
    for (int i = 0; i < tabBar->count(); ++i) {
        if (getItemId(i) == itemId)
            return i;
    }
    return -1;
}
