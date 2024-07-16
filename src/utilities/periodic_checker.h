#ifndef PERIODICCHECKER_H
#define PERIODICCHECKER_H

#include <functional>
#include <QObject>
#include <QPointer>

class PeriodicTimer;

//!
//! After \c start() is called, the predicate is invoked periodically until
//!   (1) it returns true,
//!   (2) the time-out duration has elapsed, or
//!   (3) \c cancel() is called,
//! whichever happens first. In the case (1), the callback set via \c onPredicateReturnsTrue()
//! is called; in the case (2), the callback set via \c onTimeOut() is called.
//!
//! The predicate and callbacks will run in the context of \c this.
//!
class PeriodicChecker : public QObject
{
    Q_OBJECT
public:
    explicit PeriodicChecker(QObject *parent = nullptr);

    PeriodicChecker *setPeriod(const int periodMSec);
    PeriodicChecker *setPredicate(std::function<bool ()> predicate);
    PeriodicChecker *onPredicateReturnsTrue(std::function<void()> callback);
    PeriodicChecker *setTimeOut(const int timeoutMSec);
    PeriodicChecker *onTimeOut(std::function<void()> callback);
    PeriodicChecker *setAutoDelete();

    void start();

    //!
    //! Will block until finished. Uses \c QEventLoop.
    //!
    void startAndWait(bool *isDueToTimeOut = nullptr);

    void cancel();

private:
    PeriodicTimer *mPeriodicTimer;
    int mTimeOut {1000}; // (ms)
    bool mAutoDelete {false};
    std::function<void()> mPredicateTrueCallback;
    std::function<void()> mTimeOutCallback;
};

#endif // PERIODICCHECKER_H
