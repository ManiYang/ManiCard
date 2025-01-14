#include "group_box.h"

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
    return nullptr;
}

void GroupBox::setUpContents(QGraphicsItem */*contentsContainer*/) {
    // do nothing
}

void GroupBox::adjustContents() {
    // do nothing
}

void GroupBox::onMousePressed(const bool /*isOnCaptionBar*/) {
    emit mousePressed();
}

void GroupBox::onMouseLeftClicked() {
    // do nothing
}
