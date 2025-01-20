#include "app_data_readonly.h"
#include "group_box.h"
#include "services.h"
#include <QMessageBox>

GroupBox::GroupBox(QGraphicsItem *parent)
        : BoardBoxItem(getCreationParameters(), parent) {
}

void GroupBox::setTitle(const QString &title) {
    this->title = title;

    constexpr bool bold = true;
    setCaptionBarLeftText(title, bold);
}

QString GroupBox::getTitle() const {
    return title;
}

BoardBoxItem::CreationParameters GroupBox::getCreationParameters() {
    CreationParameters parameters;

    parameters.contentsBackgroundType = ContentsBackgroundType::Transparent;
    parameters.borderShape = BorderShape::Dashed;
    parameters.highlightFrameColors = std::make_pair(QColor(186, 204, 222), QColor(58, 91, 120));

    return parameters;
}

QMenu *GroupBox::createCaptionBarContextMenu(){
    auto *contextMenu = new QMenu;
    {
        auto *action = contextMenu->addAction("Rename");
        contextMenuActionToIcon.insert(action, Icon::EditSquare);
        connect(action, &QAction::triggered, this, [this]() {
            emit userToRenameGroupBox();
        });
    }
    contextMenu->addSeparator();
    {
        auto *action = contextMenu->addAction("Remove Group Box");
        contextMenuActionToIcon.insert(action, Icon::Delete);
        connect(action, &QAction::triggered, this, [this]() {
            emit userToRemoveGroupBox();
        });
    }
    return contextMenu;
}

void GroupBox::adjustCaptionBarContextMenuBeforePopup(QMenu */*contextMenu*/) {
    // set action icons
    const auto theme = Services::instance()->getAppDataReadonly()->getIsDarkTheme()
            ? Icons::Theme::Dark : Icons::Theme::Light;
    for (auto it = contextMenuActionToIcon.constBegin();
            it != contextMenuActionToIcon.constEnd(); ++it) {
        it.key()->setIcon(Icons::getIcon(it.value(), theme));
    }
}

void GroupBox::setUpContents(QGraphicsItem */*contentsContainer*/) {
    // do nothing
}

void GroupBox::adjustContents() {
    // do nothing
}

void GroupBox::onMouseLeftPressed(
        const bool isOnCaptionBar, const Qt::KeyboardModifiers modifiers) {
    if (modifiers == Qt::NoModifier) {
        emit leftButtonPressed();
    }
    else if (modifiers == Qt::ControlModifier) {
        if (isOnCaptionBar)
            emit ctrlLeftButtonPressedOnCaptionBar();
    }
}

void GroupBox::onMouseLeftClicked(
        const bool /*isOnCaptionBar*/, const Qt::KeyboardModifiers /*modifiers*/) {
    // do nothing
}
