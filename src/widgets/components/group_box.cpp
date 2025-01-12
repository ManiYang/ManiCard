#include "group_box.h"

GroupBox::GroupBox(QGraphicsItem *parent)
        : BoardBoxItem(
              BoardBoxItem::BorderShape::Dashed,
              BoardBoxItem::ContentsBackgroundType::Transparent, parent) {
}

void GroupBox::setTitle(const QString &title) {
    constexpr bool bold = true;
    setCaptionBarLeftText(title, bold);
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


