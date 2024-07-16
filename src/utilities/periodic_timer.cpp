#include "periodic_timer.h"

PeriodicTimer::PeriodicTimer(QObject *parent)
        : QObject(parent)
        , mTimer(new QTimer(this)) {
    connect(mTimer, &QTimer::timeout, this, [this]() {
        if (mElapsedTimer.hasExpired(mTimeOut)) {
            mTimer->stop();
            emit stopped(true);
        }
        else {
            emit triggered();
        }
    });
}

PeriodicTimer::PeriodicTimer(const int periodMSec, QObject *parent)
        : PeriodicTimer(parent) {
    setPeriod(periodMSec);
}

void PeriodicTimer::setPeriod(int periodMSec) {
    mTimer->setInterval(periodMSec);
}

void PeriodicTimer::startWithTimeOut(int timeOutMSec) {
    mTimeOut = timeOutMSec;
    mElapsedTimer.start();
    mTimer->start();
}

void PeriodicTimer::stop() {
    if (mTimer->isActive()) {
        mTimer->stop();
        emit stopped(false);
    }
}

void PeriodicTimer::cancel() {
    mTimer->stop();
}
