#include <QEventLoop>
#include "periodic_checker.h"
#include "utilities/periodic_timer.h"

PeriodicChecker::PeriodicChecker(QObject *parent)
        : QObject(parent)
        , mPeriodicTimer(new PeriodicTimer(this)) {
}

PeriodicChecker *PeriodicChecker::setPeriod(const int periodMSec) {
    mPeriodicTimer->setPeriod(periodMSec);
    return this;
}

PeriodicChecker *PeriodicChecker::setPredicate(std::function<bool ()> predicate) {
    Q_ASSERT(predicate);

    connect(mPeriodicTimer, &PeriodicTimer::triggered, this, [this, predicate]() {
        if (predicate())
            mPeriodicTimer->stop();
    });
    return this;
}

PeriodicChecker *PeriodicChecker::setTimeOut(const int timeoutMSec) {
    mTimeOut = timeoutMSec;
    return this;
}

PeriodicChecker *PeriodicChecker::onPredicateReturnsTrue(std::function<void ()> callback) {
    mPredicateTrueCallback = callback;
    return this;
}

PeriodicChecker *PeriodicChecker::onTimeOut(std::function<void ()> callback) {
    mTimeOutCallback = callback;
    return this;
}

PeriodicChecker *PeriodicChecker::setAutoDelete() {
    mAutoDelete = true;
    return this;
}

void PeriodicChecker::start() {
    disconnect(mPeriodicTimer, &PeriodicTimer::stopped, nullptr, nullptr);
    connect(mPeriodicTimer, &PeriodicTimer::stopped, this, [this](const bool isDueToTimeOut) {
        if (isDueToTimeOut) {
            if (mTimeOutCallback)
                mTimeOutCallback();
        }
        else {
            if (mPredicateTrueCallback)
                mPredicateTrueCallback();
        }

        if (mAutoDelete)
            this->deleteLater();
    });

    mPeriodicTimer->startWithTimeOut(mTimeOut);
}

void PeriodicChecker::startAndWait(bool *isDueToTimeOut) {
    QEventLoop eventLoop;

    disconnect(mPeriodicTimer, &PeriodicTimer::stopped, nullptr, nullptr);
    connect(mPeriodicTimer, &PeriodicTimer::stopped,
            this, [this, isDueToTimeOut, &eventLoop](const bool isDueToTimeOut1) {
        if (isDueToTimeOut1) {
            if (mTimeOutCallback)
                mTimeOutCallback();
        }
        else {
            if (mPredicateTrueCallback)
                mPredicateTrueCallback();
        }

        if (isDueToTimeOut != nullptr)
            *isDueToTimeOut = isDueToTimeOut1;
        QTimer::singleShot(0, &eventLoop, &QEventLoop::quit);
    });

    mPeriodicTimer->startWithTimeOut(mTimeOut);
    eventLoop.exec();

    if (mAutoDelete)
        this->deleteLater();
}

void PeriodicChecker::cancel() {
    mPeriodicTimer->cancel();

    if (mAutoDelete)
        this->deleteLater();
}
