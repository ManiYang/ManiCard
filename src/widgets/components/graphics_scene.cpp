#include <QApplication>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include "graphics_scene.h"
#include "utilities/numbers_util.h"

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{}

void GraphicsScene::keyPressEvent(QKeyEvent *event) {
    const bool isSpaceKeyPressEvent
            = (event->key() == Qt::Key_Space) && !event->isAutoRepeat();
    if (isSpaceKeyPressEvent)
        isSpaceKeyPressed = true;

    //
    switch (mState) {
    case State::Normal:
        QGraphicsScene::keyPressEvent(event); // perform default behavior
        if (!event->isAccepted()
                && isSpaceKeyPressEvent
                && qApp->keyboardModifiers() == Qt::NoModifier
                && !isLeftButtonPressed) {
            mState = State::LeftDragScrollStandby;
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
    switch (mState) {
    case State::Normal: [[fallthrough]];
    case State::RightPressed:
        QGraphicsScene::keyReleaseEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling:
        event->accept();
        return;

    case State::LeftDragScrollStandby:
        if (isSpaceKeyReleaseEvent && !isLeftButtonPressed)
            mState = State::Normal;
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
    switch (mState) {
    case State::Normal:
        if (event->button() == Qt::RightButton) {
            mousePressScreenPos = event->screenPos();
            viewCenterBeforeDragScroll = getViewCenterInScene();
            mState = State::RightPressed;

            QGraphicsScene::mousePressEvent(event); // perform default behavior
        }
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
    switch (mState) {
    case State::Normal:
        QGraphicsScene::mouseMoveEvent(event); // perform default behavior
        return;

    case State::RightPressed:
    {
        constexpr double bufferDistance = 4.0;
        const auto displacement = event->screenPos() - mousePressScreenPos;
        if (vectorLength(displacement) >= bufferDistance) {
            mState = State::RightDragScrolling;
            startDragScrolling();
            onDragScrolled(viewCenterBeforeDragScroll, displacement);
        }

        QGraphicsScene::mouseMoveEvent(event); // perform default behavior
        return;
    }
    case State::RightDragScrolling:
        onDragScrolled(viewCenterBeforeDragScroll, event->screenPos() - mousePressScreenPos);
        event->accept();
        return;

    case State::LeftDragScrollStandby:
        if (isSpaceKeyPressed && isLeftButtonPressed) {
            mState = State::LeftDragScrolling;
            startDragScrolling();
            onDragScrolled(viewCenterBeforeDragScroll, event->screenPos() - mousePressScreenPos);
        }
        event->accept();
        return;

    case State::LeftDragScrolling:
        onDragScrolled(viewCenterBeforeDragScroll, event->screenPos() - mousePressScreenPos);
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        isLeftButtonPressed = false;

    //
    switch (mState) {
    case State::Normal:
        QGraphicsScene::mouseReleaseEvent(event); // perform default behavior
        return;

    case State::RightPressed:
        if (event->button() == Qt::RightButton)
            mState = State::Normal;
        QGraphicsScene::mouseReleaseEvent(event); // perform default behavior
        return;

    case State::RightDragScrolling:
        if (event->button() == Qt::RightButton) {
            endDragScolling();
            mState = State::Normal;
        }
        event->accept();
        return;

    case State::LeftDragScrollStandby:
        if (event->button() == Qt::LeftButton && !isSpaceKeyPressed)
            mState = State::Normal;
        event->accept();
        return;

    case State::LeftDragScrolling:
        if (event->button() == Qt::LeftButton) {
            endDragScolling();
            if (isSpaceKeyPressed)
                mState = State::LeftDragScrollStandby;
            else
                mState = State::Normal;
        }
        event->accept();
        return;
    }
    Q_ASSERT(false); // case not implemented
}

void GraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    switch (mState) {
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

void GraphicsScene::focusOutEvent(QFocusEvent *event) {
    switch (mState) {
    case State::Normal: [[fallthrough]];
    case State::RightPressed:
        break;

    case State::RightDragScrolling:
        endDragScolling();
        mState = State::Normal;
        break;

    case State::LeftDragScrollStandby:
        mState = State::Normal;
        break;

    case State::LeftDragScrolling:
        endDragScolling();
        mState = State::Normal;
        break;
    }

    QGraphicsScene::focusOutEvent(event);
}

void GraphicsScene::startDragScrolling() {
    if (auto *view = getView(); view != nullptr)
        view->setCursor(Qt::ClosedHandCursor);
}

void GraphicsScene::onDragScrolled(
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
        view->unsetCursor();
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
    return view->mapToScene(view->width() / 2, view->height() / 2);
}
