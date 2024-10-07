#include <QApplication>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include "graphics_scene.h"
#include "utilities/numbers_util.h"

GraphicsScene::GraphicsScene(QObject *parent)
        : QGraphicsScene(parent)
        , timerResetAccumulatedWheelDelta(new QTimer(this))
        , timerFinishViewScrolling(new QTimer(this)) {
    timerResetAccumulatedWheelDelta->setInterval(200);
    timerResetAccumulatedWheelDelta->setSingleShot(true);
    connect(timerResetAccumulatedWheelDelta, &QTimer::timeout, this, [this]() {
        accumulatedWheelDelta = 0;
    });

    timerFinishViewScrolling->setInterval(800);
    timerFinishViewScrolling->setSingleShot(true);
    connect(timerFinishViewScrolling, &QTimer::timeout, this, [this]() {
        emit viewScrollingFinished();
    });
}

void GraphicsScene::keyPressEvent(QKeyEvent *event) {
    const bool isSpaceKeyPressEvent
            = (event->key() == Qt::Key_Space) && !event->isAutoRepeat();
    if (isSpaceKeyPressEvent)
        isSpaceKeyPressed = true;

    //
    switch (state) {
    case State::Normal:
        QGraphicsScene::keyPressEvent(event); // perform default behavior
        if (!event->isAccepted()
                && isSpaceKeyPressEvent
                && qApp->keyboardModifiers() == Qt::NoModifier
                && !isLeftButtonPressed) {
            state = State::LeftDragScrollStandby;
            qDebug().noquote() << "enter LeftDragScrollStandby";
            event->accept();
        }
        return;

    case State::RightPressed:
        QGraphicsScene::keyPressEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling: [[fallthrough]];
    case State::LeftDragScrollStandby: [[fallthrough]];
    case State::LeftDragScrolling:
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::keyReleaseEvent(QKeyEvent *event) {
    const bool isSpaceKeyReleaseEvent
            = (event->key() == Qt::Key_Space) && !event->isAutoRepeat();
    if (isSpaceKeyReleaseEvent)
        isSpaceKeyPressed = false;

    //
    switch (state) {
    case State::Normal: [[fallthrough]];
    case State::RightPressed:
        QGraphicsScene::keyReleaseEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling:
        event->accept();
        return;

    case State::LeftDragScrollStandby:
        if (isSpaceKeyReleaseEvent && !isLeftButtonPressed)
            state = State::Normal;
        event->accept();
        return;

    case State::LeftDragScrolling:
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        isLeftButtonPressed = true;

    //
    switch (state) {
    case State::Normal:
        if (event->button() == Qt::RightButton) {
            mousePressScreenPos = event->screenPos();
            viewCenterBeforeDragScroll = getViewCenterInScene();
            state = State::RightPressed;
        }
        QGraphicsScene::mousePressEvent(event); // perform default behavior
        return;

    case State::RightPressed:
        QGraphicsScene::mousePressEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling:
        event->accept();
        return;
    case State::LeftDragScrollStandby:
        if (event->button() == Qt::LeftButton) {
            mousePressScreenPos = event->screenPos();
            viewCenterBeforeDragScroll = getViewCenterInScene();
        }
        event->accept();
        return;

    case State::LeftDragScrolling:
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    switch (state) {
    case State::Normal:
        QGraphicsScene::mouseMoveEvent(event); // perform default behavior
        return;

    case State::RightPressed:
    {
        constexpr double bufferDistance = 4.0;
        const auto displacement = event->screenPos() - mousePressScreenPos;
        if (vectorLength(displacement) >= bufferDistance) {
            state = State::RightDragScrolling;
            startDragScrolling();
            dragScroll(viewCenterBeforeDragScroll, displacement);
        }

        QGraphicsScene::mouseMoveEvent(event); // perform default behavior
        return;
    }
    case State::RightDragScrolling:
    {
        const auto displacement = event->screenPos() - mousePressScreenPos;
        dragScroll(viewCenterBeforeDragScroll, displacement);
        event->accept();
        return;
    }

    case State::LeftDragScrollStandby:
        if (isSpaceKeyPressed && isLeftButtonPressed) {
            state = State::LeftDragScrolling;
            startDragScrolling();
            dragScroll(viewCenterBeforeDragScroll, event->screenPos() - mousePressScreenPos);
        }
        event->accept();
        return;

    case State::LeftDragScrolling:
        dragScroll(viewCenterBeforeDragScroll, event->screenPos() - mousePressScreenPos);
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        isLeftButtonPressed = false;

    //
    switch (state) {
    case State::Normal:
        QGraphicsScene::mouseReleaseEvent(event); // perform default behavior
        if (!event->isAccepted()) {
            if (event->button() == Qt::LeftButton)
                emit clickedOnBackground();
        }
        return;

    case State::RightPressed:
        if (event->button() == Qt::RightButton)
            state = State::Normal;
        QGraphicsScene::mouseReleaseEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling:
        if (event->button() == Qt::RightButton) {
            endDragScolling();
            QTimer::singleShot(0, this, [this]() {
                state = State::Normal;
            });
            // (Use singleShot() so that the handler of contextMenuEvent following this event
            // still sees mState = State::RightDragScrolling)
        }
        event->accept();
        return;

    case State::LeftDragScrollStandby:
        if (event->button() == Qt::LeftButton && !isSpaceKeyPressed)
            state = State::Normal;
        event->accept();
        return;

    case State::LeftDragScrolling:
        if (event->button() == Qt::LeftButton) {
            endDragScolling();
            if (isSpaceKeyPressed)
                state = State::LeftDragScrollStandby;
            else
                state = State::Normal;
        }
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    switch (state) {
    case State::Normal: [[fallthrough]];
    case State::RightPressed:
        QGraphicsScene::mouseDoubleClickEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling: [[fallthrough]];
    case State::LeftDragScrollStandby: [[fallthrough]];
    case State::LeftDragScrolling:
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event) {
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->delta() == 0)
            return;

        timerResetAccumulatedWheelDelta->start();

        //
        const int delta = discretizeWheelDelta(event->delta());
                // the absolute value of `delta` is a divisor of 120
        accumulatedWheelDelta += delta;

        if (accumulatedWheelDelta >= 120) {
            accumulatedWheelDelta -= 120;
            emit userToZoomInOut(true, event->scenePos());
        }
        else if (accumulatedWheelDelta <= -120) {
            accumulatedWheelDelta += 120;
            emit userToZoomInOut(false, event->scenePos());
        }

        //
        event->accept();
        return;
    }
    else if (event->modifiers() == Qt::NoModifier) {
        QGraphicsScene::wheelEvent(event);

        if (!event->isAccepted()) {
            // assume that the QGraphicsView will be scrolled
            timerFinishViewScrolling->start();
            emit viewScrollingStarted();
        }
        return;
    }

    QGraphicsScene::wheelEvent(event);
}

void GraphicsScene::focusOutEvent(QFocusEvent *event) {
    switch (state) {
    case State::Normal: [[fallthrough]];
    case State::RightPressed:
        break;

    case State::RightDragScrolling:
        endDragScolling();
        state = State::Normal;
        break;

    case State::LeftDragScrollStandby:
        state = State::Normal;
        break;

    case State::LeftDragScrolling:
        endDragScolling();
        state = State::Normal;
        break;
    }

    QGraphicsScene::focusOutEvent(event);
}

void GraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    if (state == State::Normal) {
        QGraphicsScene::contextMenuEvent(event);

        if (!event->isAccepted()) {
            emit contextMenuRequestedOnScene(event->scenePos());
            event->accept();
        }
    }
}

void GraphicsScene::startDragScrolling() {
    if (auto *view = getView(); view != nullptr)
        view->viewport()->setCursor(Qt::ClosedHandCursor);
}

void GraphicsScene::dragScroll(
        const QPointF &viewCenterBeforeDragScroll, const QPoint &dragDisplacement) {
    QGraphicsView *view = getView();
    if (view == nullptr)
        return;

    const QRectF sceneRect = view->sceneRect();
    const double viewHalfWidth = view->width() / 2.0;
    const double viewHalfHeight = view->height() / 2.0;

    QPointF newCenter = viewCenterBeforeDragScroll - dragDisplacement;
    if (const double xMax = sceneRect.right() - viewHalfWidth; newCenter.x() > xMax)
        newCenter.setX(xMax);
    if (const double xMin = sceneRect.left() + viewHalfWidth; newCenter.x() < xMin)
        newCenter.setX(xMin);
    if (const double yMax = sceneRect.bottom() - viewHalfHeight; newCenter.y() > yMax)
        newCenter.setY(yMax);
    if (const double yMin = sceneRect.top() + viewHalfHeight; newCenter.y() < yMin)
        newCenter.setY(yMin);

    view->centerOn(newCenter);
}

void GraphicsScene::endDragScolling() {
    if (auto *view = getView(); view != nullptr)
        view->viewport()->unsetCursor();
    emit dragScrollingEnded();
}

QGraphicsView *GraphicsScene::getView() const {
    if (views().isEmpty())
        return nullptr;
    return views().at(0); // assuming there's only one view
}

QPointF GraphicsScene::getViewCenterInScene() const {
    QGraphicsView *view = getView();
    if (view == nullptr)
        return {0.0, 0.0};
    return view->mapToScene(view->viewport()->width() / 2, view->viewport()->height() / 2);
}

int GraphicsScene::discretizeWheelDelta(const int delta) {
    if (delta >= 90)
        return 120;
    else if (delta <= -90)
        return -120;
    else if (delta >= 45)
        return 60;
    else if (delta <= -45)
        return -60;
    else if (delta >= 20)
        return 30;
    else if (delta <= -20)
        return -30;
    else if (delta >= 5)
        return 10;
    else if (delta <= -5)
        return -10;
    else // (delta is within [-4, 4])
        return delta;
}
