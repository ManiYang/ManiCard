#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "board_box_item.h"
#include "utilities/margins_util.h"
#include "widgets/components/graphics_item_move_resize.h"

BoardBoxItem::BoardBoxItem(const BorderShape borderShape, QGraphicsItem *parent)
        : QGraphicsObject(parent)
        , borderShape(borderShape)
        , captionBarItem(new QGraphicsRectItem(this))
        , captionBarLeftTextItem(new QGraphicsSimpleTextItem(captionBarItem))
        , captionBarRightTextItem(new QGraphicsSimpleTextItem(captionBarItem))
        , contentsRectItem(new QGraphicsRectItem(this))
        , moveResizeHelper(new GraphicsItemMoveResize(this)) {
}

BoardBoxItem::~BoardBoxItem() {
    if (captionBarContextMenu != nullptr)
        captionBarContextMenu->deleteLater();
}

void BoardBoxItem::initialize() {
    Q_ASSERT(scene() != nullptr);

    setUpGraphicsItems(); // this calls setUpContents()
    adjustGraphicsItems(); // this calls adjustContents()

    //
    captionBarContextMenu = createCaptionBarContextMenu(); // can be nullptr

    // move-resize helper
    moveResizeHelper->setMoveHandle(captionBarItem);

    constexpr double resizeAreaMaxWidth = 6.0; // in pixel
    moveResizeHelper->setResizeHandle(this, resizeAreaMaxWidth, minSizeForResizing);

    //
    installEventFilterOnChildItems();

    //
    setUpConnections();
}

void BoardBoxItem::setRect(const QRectF &rect) {
    prepareGeometryChange();
    borderOuterRect = rect;
    update();
    adjustGraphicsItems();
}

void BoardBoxItem::setMarginWidth(const double width) {
    marginWidth = width;
    update();
    adjustGraphicsItems();
}

void BoardBoxItem::setBorderWidth(const double width) {
    borderWidth = width;
    update();
    adjustGraphicsItems();
}

void BoardBoxItem::setColor(const QColor &color_) {
    color = color_;
    update();
    adjustGraphicsItems();
}

void BoardBoxItem::setIsHighlighted(const bool isHighlighted_) {
    isHighlighted = isHighlighted_;
    update();
}

QRectF BoardBoxItem::getRect() const {
    return borderOuterRect;
}

bool BoardBoxItem::getIsHighlighted() const {
    return isHighlighted;
}

QRectF BoardBoxItem::boundingRect() const {
    return borderOuterRect.marginsAdded(uniformMarginsF(marginWidth));
}

void BoardBoxItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    painter->save();

    // draw border rect
    {
        std::optional<Qt::PenStyle> penStyle;
        switch (borderShape) {
        case BorderShape::Solid:
            penStyle = Qt::SolidLine;
            break;
        case BorderShape::Dashed:
            penStyle = Qt::DotLine;
            break;
        }
        if (!penStyle.has_value()) {
            Q_ASSERT(false); // case not implemented
            penStyle = Qt::SolidLine;
        }

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen {QBrush(color), borderWidth, penStyle.value()});
        const double radius = borderWidth;
        painter->drawRoundedRect(
                borderOuterRect.marginsRemoved(uniformMarginsF(borderWidth / 2.0)),
                radius, radius);
    }

    // draw highlight box
    if (isHighlighted) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(getHighlightBoxColor(color), highlightBoxWidth));
        const double radius = borderWidth;
        painter->drawRoundedRect(
                borderOuterRect.marginsAdded(uniformMarginsF(marginWidth))
                    .marginsRemoved(uniformMarginsF(highlightBoxWidth / 2.0)),
                radius, radius);
    }

    //
    painter->restore();
}

void BoardBoxItem::setCaptionBarLeftText(const QString &text) {
    captionBarLeftTextItem->setText(text);
}

void BoardBoxItem::setCaptionBarLeftText(const QString &text, const bool bold) {
    captionBarLeftTextItem->setText(text);

    //
    QFont font = captionBarLeftTextItem->font();
    font.setBold(bold);
    captionBarLeftTextItem->setFont(font);
}

void BoardBoxItem::setCaptionBarRightText(const QString &text) {
    captionBarRightTextItem->setText(text);
    setCaptionBarRightTextItemPos(captionBarRect, captionBarPadding);
}

void BoardBoxItem::setCaptionBarRightText(const QString &text, const bool bold) {
    captionBarRightTextItem->setText(text);

    //
    QFont font = captionBarRightTextItem->font();
    font.setBold(bold);
    captionBarRightTextItem->setFont(font);

    //
    setCaptionBarRightTextItemPos(captionBarRect, captionBarPadding);
}

QRectF BoardBoxItem::getContentsRect() const {
    return contentsRectItem->rect();
}

QGraphicsView *BoardBoxItem::getView() const {
    if (scene() == nullptr)
        return nullptr;

    if (const auto views = scene()->views(); !views.isEmpty())
        return views.at(0);
    else
        return nullptr;
}

void BoardBoxItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    event->accept();
}

bool BoardBoxItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    if (watched == captionBarItem
            || watched == captionBarLeftTextItem
            || watched == captionBarRightTextItem) {
        if (event->type() == QEvent::GraphicsSceneContextMenu) {
            auto *e = dynamic_cast<QGraphicsSceneContextMenuEvent *>(event);
            const auto pos = e->screenPos();
            if (captionBarContextMenu != nullptr)
                captionBarContextMenu->popup(pos);
            return true;
        }
        else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            emit clicked();
        }
    }

    return false;
}

void BoardBoxItem::mousePressEvent(QGraphicsSceneMouseEvent */*event*/) {
    // do nothing

    // This method is reimplemented so that
    // 1. the mouse-press event is accepted and `this` becomes the mouse grabber, and
    // 2. the later mouse-release event will be sent to `this` and will not "penetrate"
    //    through `this` and reach the QGraphicsScene.
}

void BoardBoxItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsObject::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton)
        emit clicked();
}

void BoardBoxItem::setUpConnections() {
    // ==== moveResizeHelper ====

    connect(moveResizeHelper, &GraphicsItemMoveResize::getTargetItemPosition,
            this, [this](QPointF *pos) {
        *pos = this->mapToScene(borderOuterRect.topLeft());
    }, Qt::DirectConnection);

    connect(moveResizeHelper, &GraphicsItemMoveResize::setTargetItemPosition,
            this, [this](const QPointF &pos) {
        prepareGeometryChange();
        borderOuterRect.moveTopLeft(this->mapFromScene(pos));
        update();
        adjustGraphicsItems();

        scene()->invalidate(QRectF(), QGraphicsScene::BackgroundLayer);
        // this is to deal with the QGraphicsView problem
        // https://forum.qt.io/topic/157478/qgraphicsscene-incorrect-artifacts-on-scrolling-bug

        emit movedOrResized();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::movingEnded, this, [this]() {
        emit finishedMovingOrResizing();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::getTargetItemRect,
            this, [this](QRectF *rect) {
        *rect = QRectF(
            mapToScene(borderOuterRect.topLeft()),
            mapToScene(borderOuterRect.bottomRight()));
    }, Qt::DirectConnection);

    connect(moveResizeHelper, &GraphicsItemMoveResize::setTargetItemRect,
            this, [this](const QRectF &rect) {
        prepareGeometryChange();
        borderOuterRect = QRectF(
                mapFromScene(rect.topLeft()),
                mapFromScene(rect.bottomRight()));
        update();
        adjustGraphicsItems();

        emit movedOrResized();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::resizingEnded, this, [this]() {
        emit finishedMovingOrResizing();
    });

    connect(moveResizeHelper, &GraphicsItemMoveResize::setCursorShape,
            this, [this](const std::optional<Qt::CursorShape> cursorShape) {
        if (cursorShape.has_value())
            setCursor(cursorShape.value());
        else
            unsetCursor();
    });
}

void BoardBoxItem::installEventFilterOnChildItems() {
    captionBarItem->installSceneEventFilter(this);
    captionBarLeftTextItem->installSceneEventFilter(this);
    captionBarRightTextItem->installSceneEventFilter(this);
}

void BoardBoxItem::setUpGraphicsItems() {
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setAcceptHoverEvents(true);

    // caption bar
    const QColor captionBarTextColor(Qt::white);

    QFont captionBarFontNormal;
    {
        constexpr int captionBarFontPixelSize = 13;
        const QString captionBarFontFamily = "Arial";


        captionBarFontNormal.setFamily(captionBarFontFamily);
        captionBarFontNormal.setPixelSize(captionBarFontPixelSize);

        captionBarFontHeight = QFontMetrics(captionBarFontNormal).height();
    }

    // caption bar left text
    captionBarLeftTextItem->setFont(captionBarFontNormal);
    captionBarLeftTextItem->setBrush(captionBarTextColor);

    // caption bar right text
    captionBarRightTextItem->setFont(captionBarFontNormal);
    captionBarRightTextItem->setBrush(captionBarTextColor);

    // contents rect
    contentsRectItem->setPen(Qt::NoPen);
    contentsRectItem->setBrush(Qt::white);
    contentsRectItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    //
    setUpContents(contentsRectItem);
}

void BoardBoxItem::adjustGraphicsItems() {
    const auto borderInnerRect
            = borderOuterRect.marginsRemoved(uniformMarginsF(borderWidth));

    // caption bar
    {
        captionBarRect = QRectF(
                borderInnerRect.topLeft(),
                QSizeF(borderInnerRect.width(), captionBarFontHeight + captionBarPadding * 2));

        captionBarItem->setRect(captionBarRect);
        captionBarItem->setPen(Qt::NoPen);
        captionBarItem->setBrush(color);
        captionBarItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    }

    // caption bar left text
    captionBarLeftTextItem->setPos(
            captionBarRect.topLeft() + QPointF(captionBarPadding, captionBarPadding));

    // caption bar right text
    setCaptionBarRightTextItemPos(captionBarRect, captionBarPadding);

    // contents rect
    contentsRectItem->setRect(
            borderInnerRect.marginsRemoved({0.0, captionBarRect.height(), 0.0, 0.0})); // <^>v

    // contents
    adjustContents();
}

void BoardBoxItem::setCaptionBarRightTextItemPos(
        const QRectF captionBarRect, const double captionBarPadding) {
    const double x
            = captionBarRect.right() - captionBarPadding
              - captionBarRightTextItem->boundingRect().width();
    captionBarRightTextItem->setPos(x, captionBarRect.top() + captionBarPadding);
}

QColor BoardBoxItem::getHighlightBoxColor(const QColor &color) {
    int h, s, v;
    color.getHsv(&h, &s, &v);

    if (h >= 180 && h <= 240
            && s >= 50
            && v >= 60) {
        return QColor(90, 90, 90);
    }
    else {
        return QColor(36, 128, 220); // hue = 210
    }
}

QMenu *BoardBoxItem::createCaptionBarContextMenu() {
    return nullptr;
}

void BoardBoxItem::setUpContents(QGraphicsItem */*contentsContainer*/) {
    // do nothing
}

void BoardBoxItem::adjustContents() {
    // do nothing
}
