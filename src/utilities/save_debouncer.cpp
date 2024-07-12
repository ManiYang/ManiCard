#include "save_debouncer.h"

SaveDebouncer::SaveDebouncer(const int delayInterval, QObject *parent)
        : QObject(parent)
        , timer(new QTimer(this)) {
    timer->setInterval(delayInterval);
    connect(timer, &QTimer::timeout, this, [this]() { onTimerTimeout(); });
}

void SaveDebouncer::setUpdated() {
    updated = true;
    if (state == State::Clear) {
        timer->start();
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

bool SaveDebouncer::isCleared() const {
    return state == State::Clear;
}

void SaveDebouncer::onTimerTimeout() {
    if (state == State::Pending) {
        updated = false;
        state = State::Saving;

        emit saveCurrentState();
        // this goes last because the receiver of this signal may call saveFinished()
        // synchronously
    }
}
