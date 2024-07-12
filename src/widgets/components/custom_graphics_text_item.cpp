#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QTextCursor>
#include <QTextDocument>
#include "custom_graphics_text_item.h"

CustomGraphicsTextItem::CustomGraphicsTextItem(QGraphicsItem *parent)
        : QGraphicsObject(parent)
        , graphicsTextItem(new GraphicsTextItemTweak(this)) {
    setFlag(QGraphicsItem::ItemHasNoContents);
    graphicsTextItem->setPos(0, 0);
    height = graphicsTextItem->boundingRect().height();

    connect(graphicsTextItem->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (!textChangeIsByUser)
            return;

        bool heightChanged = false; // is height changed because of editing
        const double newHeight = graphicsTextItem->boundingRect().height();
        if (newHeight != height) {
            height = newHeight;
            heightChanged = true;
        }

        emit textEdited(heightChanged);
    });
}

void CustomGraphicsTextItem::setPlainText(const QString &text) {
    textChangeIsByUser = false;
    graphicsTextItem->setPlainText(text);
    textChangeIsByUser = true;

    height = graphicsTextItem->boundingRect().height();
}

void CustomGraphicsTextItem::setEditable(const bool editable) {
    graphicsTextItem->setTextInteractionFlags(
            editable ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
    graphicsTextItem->setCursor(editable ? Qt::IBeamCursor : Qt::ArrowCursor);
}

void CustomGraphicsTextItem::setTextWidth(const double width) {
    graphicsTextItem->setTextWidth(width);
    height = graphicsTextItem->boundingRect().height();
}

void CustomGraphicsTextItem::setFont(const QFont &font) {
    graphicsTextItem->setFont(font);
    height = graphicsTextItem->boundingRect().height();
}

void CustomGraphicsTextItem::setDefaultTextColor(const QColor &color) {
    graphicsTextItem->setDefaultTextColor(color);
}

QString CustomGraphicsTextItem::toPlainText() const {
    return graphicsTextItem->toPlainText();
}

QRectF CustomGraphicsTextItem::boundingRect() const {
    return graphicsTextItem->boundingRect();
}

void CustomGraphicsTextItem::paint(
        QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/,
        QWidget */*widget*/) {
    // do nothing
}

//====

GraphicsTextItemTweak::GraphicsTextItemTweak(QGraphicsItem *parent)
        : QGraphicsTextItem(parent) {
}

void GraphicsTextItemTweak::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // adjust the options in order to disable the dashed frame around the text when editing
    QStyleOptionGraphicsItem optionAdjusted {*option};
    optionAdjusted.state.setFlag(QStyle::State_HasFocus, false);

    //
    QGraphicsTextItem::paint(painter, &optionAdjusted, widget);
}

void GraphicsTextItemTweak::focusOutEvent(QFocusEvent *event) {
    auto cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor);
    }

    QGraphicsTextItem::focusOutEvent(event);
}
