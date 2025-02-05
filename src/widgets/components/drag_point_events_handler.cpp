#include <QDebug>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include "drag_point_events_handler.h"

DragPointEventsHandler::DragPointEventsHandler(QGraphicsItem *parent)
        : QGraphicsObject(parent) {
    setFlag(QGraphicsItem::ItemHasNoContents, true);
}

void DragPointEventsHandler::installOnItem(QGraphicsItem *item, const int itemId) {
    item->installSceneEventFilter(this);
    watchedItems.insert(item, itemId);
}

void DragPointEventsHandler::removeItem(QGraphicsItem *item) {
    watchedItems.remove(item);
}

QRectF DragPointEventsHandler::boundingRect() const {
    return {};
}

void DragPointEventsHandler::paint(
        QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    // do nothing
}

bool DragPointEventsHandler::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
    if (!watchedItems.contains(watched))
        return false;

    const int itemId = watchedItems.value(watched);

    if (event->type() == QEvent::GraphicsSceneMousePress) {
        auto *e = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(e != nullptr);
        if (e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier) {
            event->accept(); // `watched` becomes the mouse grabber

            EventParameters params;
            params.mousePos = e->pos();
            params.itemId = itemId;

            handleEvent(Event::MousePress, params);
            return true;
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseMove) {
        auto *e = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(e != nullptr);

        EventParameters params;
        params.mousePos = e->pos();
        params.itemId = itemId;

        handleEvent(Event::MouseMove, params);
        return true;
    }
    else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        auto *e = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(e != nullptr);
        if (e->button() == Qt::LeftButton) {
            EventParameters params;
            params.mousePos = e->pos();
            params.itemId = itemId;

            handleEvent(Event::MouseRelease, params);
            return true;
        }
    }
    else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        auto *e = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        Q_ASSERT(e != nullptr);
        if (e->button() == Qt::LeftButton) {
            EventParameters params;
            params.mousePos = e->pos();
            params.itemId = itemId;

            handleEvent(Event::MouseDoubleClick, params);
            return true;
        }
    }

    return false;
}

void DragPointEventsHandler::handleEvent(const Event event, const EventParameters &parameters) {
    switch (state) {
    case DragPointEventsHandler::State::Normal:
        if (event == Event::MousePress) {
            mousePressPos = parameters.mousePos;

            QPointF itemPos {1e10, 0.0};
            emit getPosition(parameters.itemId, &itemPos);
            Q_ASSERT(itemPos.x() < 1e10);
            itemPosAtMousePress = itemPos;

            state = State::Pressed;
            activeItemId = parameters.itemId;
        }
        else if (event == Event::MouseDoubleClick) {
            emit doubleClicked(parameters.itemId);
        }
        return;

    case DragPointEventsHandler::State::Pressed:
        if (event == Event::MouseRelease) {
            state = State::Normal;
            activeItemId = std::nullopt;
        }
        else if (event == Event::MouseMove) {
            if ((parameters.mousePos - mousePressPos).manhattanLength() >= 4.0) {
                if (parameters.itemId == activeItemId) {
                    state = State::Moving;
                    emit movingStarted(parameters.itemId);
                }
                else {
                    qWarning().noquote()
                            << QString("received mouse-move event for an item without "
                                       "first receiving its mouse-press event");
                }
            }
        }
        return;

    case DragPointEventsHandler::State::Moving:
        if (event == Event::MouseRelease) {
            state = State::Normal;
            activeItemId = std::nullopt;
            emit movingFinished(parameters.itemId);
        }
        else if (event == Event::MouseMove) {
            if (parameters.itemId == activeItemId) {
                const QPointF newItemPos
                        = itemPosAtMousePress + parameters.mousePos - mousePressPos;
                emit positionUpdated(parameters.itemId, newItemPos);
            }
            else {
                qWarning().noquote()
                        << QString("received mouse-release event for an item without "
                                   "first receiving its mouse-press event");
            }
        }
        return;
    }
    Q_ASSERT(false); // case not implemented
}
