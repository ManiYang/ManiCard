#include "saving_debouncer.h"

SavingDebouncer::SavingDebouncer(const int checkInterval, QObject *parent)
        : QObject(parent)
        , timer(new QTimer(this)) {
    timer->setInterval(checkInterval);
    connect(timer, &QTimer::timeout, this, [this]() {
        onTimerTimeout();
    });
}

void SavingDebouncer::setUpdated() {
    updated = true;
    if (state == State::Clear) {
        timer->start();
        state = State::Pending;
        emit savingScheduled();
    }
}

void SavingDebouncer::savingFinished() {
    if (updated) {
        state = State::Pending;
    }
    else {
        timer->stop();
        state = State::Clear;
        emit cleared();
    }
}

bool SavingDebouncer::isCleared() const {
    return state == State::Clear;
}

void SavingDebouncer::onTimerTimeout() {
    if (state == State::Pending) {
        updated = false;
        state = State::Saving;

        emit saveCurrentState();
        // this goes last because the receiver of this signal may call savingFinished()
        // synchronously
    }
}
