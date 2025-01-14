#include "group_box.h"

#include <QMessageBox>

GroupBox::GroupBox(QGraphicsItem *parent)
        : BoardBoxItem(getCreationParameters(), parent) {
}

void GroupBox::setTitle(const QString &title) {
    constexpr bool bold = true;
    setCaptionBarLeftText(title, bold);
}

BoardBoxItem::CreationParameters GroupBox::getCreationParameters() {
    CreationParameters parameters;

    parameters.contentsBackgroundType = ContentsBackgroundType::Transparent;
    parameters.borderShape = BorderShape::Dashed;
    parameters.highlightFrameColor = QColor(186, 204, 222);

    return parameters;
}

QMenu *GroupBox::createCaptionBarContextMenu(){
    auto *contextMenu = new QMenu;
    {
        auto *action = contextMenu->addAction(QIcon(":/icons/delete_black_24"), "Ungroup");
        connect(action, &QAction::triggered, this, [this]() {
            emit userToRemoveGroupBox();
        });
    }
    return contextMenu;
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
