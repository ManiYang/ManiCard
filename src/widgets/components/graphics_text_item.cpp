#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QTextCursor>
#include <QTextDocument>
#include "graphics_text_item.h"

GraphicsTextItem::GraphicsTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
{
    QTextDocument *doc = document();
    connect(doc, &QTextDocument::contentsChanged, this, [this]() {
        onContentsChanged();
    });

    //
    height = boundingRect().height();
}

void GraphicsTextItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // adjust the options in order to disable the dashed frame around the text when editing
    QStyleOptionGraphicsItem optionAdjusted {*option};
    optionAdjusted.state.setFlag(QStyle::State_HasFocus, false);

    //
    QGraphicsTextItem::paint(painter, &optionAdjusted, widget);
}

void GraphicsTextItem::focusOutEvent(QFocusEvent *event) {
    auto cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor);
    }

    QGraphicsTextItem::focusOutEvent(event);
}

void GraphicsTextItem::onContentsChanged() {
    bool heightChanged = false;
    double newHeight = boundingRect().height();
    if (newHeight != height) {
        heightChanged = true;
        height = newHeight;

        // Here it is assumed that only document contents change causes height change.
        // Otherwise, `height` may not reflect every height change.
    }

    emit contentChanged(heightChanged);
}
