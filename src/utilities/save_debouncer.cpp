#include "save_debouncer.h"

SaveDebouncer::SaveDebouncer(const int delayInterval_, QObject *parent)
        : QObject(parent)
        , timer(new QTimer(this))
        , delayInterval(delayInterval_) {
    connect(timer, &QTimer::timeout, this, [this]() { onTimerTimeout(); });
}

void SaveDebouncer::setUpdated() {
    updated = true;
    if (state == State::Clear) {
        timer->start(delayInterval);
        state = State::Pending;
        emit saveScheduled();
    }
}

void SaveDebouncer::saveFinished() {
    if (updated) {
        state = State::Pending;
    }
    else {
        timer->stop();
        state = State::Clear;
        emit cleared();
    }
}

void SaveDebouncer::saveNow() {
    if (state == State::Clear)
        return;

    timer->setInterval(10);
    if (state == State::Pending)
        enterSavingState();
}

bool SaveDebouncer::isCleared() const {
    return state == State::Clear;
}

void SaveDebouncer::onTimerTimeout() {
    if (state == State::Pending)
        enterSavingState();
}

void SaveDebouncer::enterSavingState() {
    Q_ASSERT(state != State::Saving);

    state = State::Saving;
    updated = false;

    emit saveCurrentState();
    // this goes last because the receiver of this signal may call saveFinished()
    // synchronously
}
