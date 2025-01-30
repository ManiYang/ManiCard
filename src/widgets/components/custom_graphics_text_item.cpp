#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
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

    graphicsTextItem->setTextInteractionFlags(Qt::NoTextInteraction);

    //
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

    connect(graphicsTextItem, &GraphicsTextItemTweak::mouseReleased, this, [this]() {
        emit clicked();
    });

    connect(graphicsTextItem, &GraphicsTextItemTweak::tabKeyPressed, this, [this]() {
        emit tabKeyPressed();
    });
}

void CustomGraphicsTextItem::setPlainText(const QString &text) {
    textChangeIsByUser = false;
    graphicsTextItem->setPlainText(text);
    textChangeIsByUser = true;

    height = graphicsTextItem->boundingRect().height();
}

void CustomGraphicsTextItem::setTextInteractionState(const TextInteractionState state) {
    textInteractionState = state;

    //
    std::optional<Qt::TextInteractionFlags> flags;
    switch (state) {
    case TextInteractionState::None:
        flags = Qt::NoTextInteraction;
        break;
    case TextInteractionState::Selectable:
        flags = Qt::TextSelectableByMouse;
        break;
    case TextInteractionState::Editable:
        flags = Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::TextEditable;
        break;
    }
    if (!flags.has_value()) {
        Q_ASSERT(false); // case not implemented
        flags = Qt::NoTextInteraction;
    }

    graphicsTextItem->setTextInteractionFlags(flags.value());

    //
    const bool iBeamCursor
            = flags.value().testFlag(Qt::TextEditable)
              || flags.value().testFlag(Qt::TextSelectableByMouse);
    graphicsTextItem->setCursor(iBeamCursor ? Qt::IBeamCursor : Qt::ArrowCursor);
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

void CustomGraphicsTextItem::setEnableContextMenu(const bool enable) {
    graphicsTextItem->setEnableContextMenu(enable);
}

QString CustomGraphicsTextItem::toPlainText() const {
    return graphicsTextItem->toPlainText();
}

QFont CustomGraphicsTextItem::font() const {
    return graphicsTextItem->font();
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

void GraphicsTextItemTweak::setEnableContextMenu(const bool enable) {
    enableContextMenu = enable;
}

void GraphicsTextItemTweak::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // adjust the options in order to disable the dashed frame around the text when editing
    QStyleOptionGraphicsItem optionAdjusted {*option};
    optionAdjusted.state.setFlag(QStyle::State_HasFocus, false);

    //
    QGraphicsTextItem::paint(painter, &optionAdjusted, widget);
}

bool GraphicsTextItemTweak::event(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        const auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            emit tabKeyPressed();
            return true;
        }
    }
    return QGraphicsTextItem::event(event);
}

void GraphicsTextItemTweak::focusOutEvent(QFocusEvent *event) {
    auto cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor);
    }

    QGraphicsTextItem::focusOutEvent(event);
}

void GraphicsTextItemTweak::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsTextItem::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
        emit mouseReleased();
}

void GraphicsTextItemTweak::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    if (enableContextMenu)
        QGraphicsTextItem::contextMenuEvent(event);
    else
        event->ignore();
}

