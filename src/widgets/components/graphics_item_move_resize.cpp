#include <QDebug>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include "graphics_item_move_resize.h"
#include "utilities/numbers_util.h"

GraphicsItemMoveResize::GraphicsItemMoveResize(QGraphicsItem *parent)
       : QGraphicsObject(parent) {
    setFlag(QGraphicsItem::ItemHasNoContents, true);
}

void GraphicsItemMoveResize::setMoveHandle(QGraphicsItem *item) {
    Q_ASSERT(item != nullptr);
    moveHandle = item;
    moveHandle->installSceneEventFilter(this);
}

void GraphicsItemMoveResize::setResizeHandle(
        QGraphicsItem *item, const double borderWidth, const QSizeF &targetItemMinimumSize) {
    Q_ASSERT(item != nullptr);
    resizeHandle = item;
    resizeHandle->installSceneEventFilter(this);
    resizeHandleBorderWidth = borderWidth;
    targetItemMinSize = targetItemMinimumSize;
}

QRectF GraphicsItemMoveResize::boundingRect() const {
    return {};
}

void GraphicsItemMoveResize::paint(
        QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    // do nothing
}

bool GraphicsItemMoveResize::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    if (watched == moveHandle)
        return eventFilterForMoveHandle(event);
    else if (watched == resizeHandle)
        return eventFilterForResizeHandle(event);
    else
        return false;
}

bool GraphicsItemMoveResize::eventFilterForMoveHandle(QEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMousePress) {
        auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(mouseEvent != nullptr);

        if (mouseEvent->button() == Qt::LeftButton
                && !(mouseEvent->buttons() & (~Qt::LeftButton))
                && mouseEvent->modifiers() == Qt::NoModifier) { // (pure left btn press)
            if (state == State::Normal) {
                QPointF itemPosition(1.1e10, 0);
                emit getTargetItemPosition(&itemPosition);
                if (itemPosition.x() <= 1e10) { // got item position
                    mousePressScreenPos = mouseEvent->screenPos();
                    targetItemPosBeforeMove = itemPosition;
                    state = State::BeforeMove;
                    return true;
                }
            }
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseMove) {
        if (state == State::BeforeMove) {
            auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
            Q_ASSERT(mouseEvent != nullptr);

            const QPoint displacement = mouseEvent->screenPos() - mousePressScreenPos; // (pix)
            constexpr double bufferDistance = 4.0;
            if (vectorLength(displacement) >= bufferDistance) {
                emit aboutToMove();
                emit setTargetItemPosition(targetItemPosBeforeMove + displacement);
                state = State::Moving;
            }
            return true;
        }
        else if (state == State::Moving) {
            auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
            Q_ASSERT(mouseEvent != nullptr);

            const QPoint displacement = mouseEvent->screenPos() - mousePressScreenPos; // (pix)
            emit setTargetItemPosition(targetItemPosBeforeMove + displacement);
            return true;
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(mouseEvent != nullptr);
        if (mouseEvent->button() == Qt::LeftButton) { // (left btn release)
            if (state == State::Moving) {
                state = State::Normal;
                emit movingEnded();
                return true;
            }
            else if (state == State::BeforeMove) {
                state = State::Normal;
                return true;
            }
        }
    }

    return false;
}

bool GraphicsItemMoveResize::eventFilterForResizeHandle(QEvent *event) {
    if (event->type() == QEvent::GraphicsSceneHoverMove) {
        if (state == State::Normal) {
            auto *hoverEvent = dynamic_cast<QGraphicsSceneHoverEvent *>(event);
            Q_ASSERT(hoverEvent != nullptr);

            const auto scenePos = hoverEvent->scenePos();
            if (isWithinResizeHandleBorder(scenePos))
                resizeDirection = getRegionInResizeHandle(scenePos);
            else
                resizeDirection = "";

            Qt::CursorShape cursorShape = Qt::ArrowCursor;
            if (resizeDirection == "NW" || resizeDirection == "SE")
                cursorShape = Qt::SizeFDiagCursor;
            else if (resizeDirection == "N" || resizeDirection == "S")
                cursorShape = Qt::SizeVerCursor;
            else if (resizeDirection == "NE" || resizeDirection == "SW")
                cursorShape = Qt::SizeBDiagCursor;
            else if (resizeDirection == "W" || resizeDirection == "E")
                cursorShape = Qt::SizeHorCursor;
            emit setCursorShape(cursorShape);

            return true;
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMousePress) {
        auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(mouseEvent != nullptr);
        if (mouseEvent->button() == Qt::LeftButton
                && !(mouseEvent->buttons() & (~Qt::LeftButton))
                && mouseEvent->modifiers() == Qt::NoModifier) { // (pure left btn press)
            if (state == State::Normal && !resizeDirection.isEmpty()) {
                QRectF itemRect;
                emit getTargetItemRect(&itemRect);
                if (!itemRect.isNull()) {
                    mousePressScreenPos = mouseEvent->screenPos();
                    targetItemRectBeforeResize = itemRect;
                    state = State::Resizing;
                    return true;
                }
            }
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseMove) {
        if (state == State::Resizing) {
            auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
            Q_ASSERT(mouseEvent != nullptr);
            const QPoint displacement = mouseEvent->screenPos() - mousePressScreenPos; // (pix)
            doResize(displacement);
            return true;
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        auto *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(mouseEvent != nullptr);
        if (mouseEvent->button() == Qt::LeftButton) { // (left btn release)
            if (state == State::Resizing) {
                state = State::Normal;
                emit resizingEnded();
                return true;
            }
        }
    }

    return false;
}

void GraphicsItemMoveResize::doResize(const QPoint displacement) {
    QRectF rect = targetItemRectBeforeResize; // (pix)

    if (resizeDirection.contains('E')) {
        rect.setRight(
                std::max(
                        targetItemRectBeforeResize.right() + displacement.x(),
                        targetItemRectBeforeResize.left() + targetItemMinSize.width()
                )
        );
    }
    else if (resizeDirection.contains('W')) {
        rect.setLeft(
                std::min(
                        targetItemRectBeforeResize.left() + displacement.x(),
                        targetItemRectBeforeResize.right() - targetItemMinSize.width()
                )
        );
    }

    if (resizeDirection.contains('S')) {
        rect.setBottom(
                std::max(
                        targetItemRectBeforeResize.bottom() + displacement.y(),
                        targetItemRectBeforeResize.top() + targetItemMinSize.height()
                )
        );
    }
    else if (resizeDirection.contains('N')) {
        rect.setTop(
                std::min(
                        targetItemRectBeforeResize.top() + displacement.y(),
                        targetItemRectBeforeResize.bottom() - targetItemMinSize.height()
                )
        );
    }

    emit setTargetItemRect(rect);
}

bool GraphicsItemMoveResize::isWithinResizeHandleBorder(const QPointF scenePos) const {
    const QRectF handleRect = QRectF(
            resizeHandle->mapToScene(resizeHandle->boundingRect().topLeft()),
            resizeHandle->mapToScene(resizeHandle->boundingRect().bottomRight()));
            // in scene coordinates

    const auto w = resizeHandleBorderWidth;
    return !handleRect.marginsRemoved({w, w, w, w}).contains(scenePos);
}

QString GraphicsItemMoveResize::getRegionInResizeHandle(const QPointF scenePos) const {
    const QRectF handleRect = QRectF(
            resizeHandle->mapToScene(resizeHandle->boundingRect().topLeft()),
            resizeHandle->mapToScene(resizeHandle->boundingRect().bottomRight()));
            // in scene coordinates

    QString result;
    if (scenePos.y() < handleRect.top() + 16)
        result += 'N';
    else if (scenePos.y() > handleRect.bottom() - 16)
        result += 'S';

    if (scenePos.x() < handleRect.left() + 16)
        result += 'W';
    else if (scenePos.x() > handleRect.right() - 16)
        result += 'E';

    return result;
}
