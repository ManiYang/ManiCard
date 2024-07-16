#ifndef PERIODICTIMER_H
#define PERIODICTIMER_H

#include <QElapsedTimer>
#include <QTimer>

//!
//! After \c startWithTimeOut(timeOut) is called, the periodic timer triggers periodically until
//!   (1) \e timeOut has elapsed,
//!   (2) \c stop() is called, or
//!   (3) \c cancel() is called,
//! whichever happens first. In the case (1), signal \c stopped(true) is emitted; in the case (2),
//! \c stopped(false) is emitted.
//!
class PeriodicTimer : public QObject
{
    Q_OBJECT
public:
    explicit PeriodicTimer(QObject *parent = nullptr);
    explicit PeriodicTimer(const int periodMSec, QObject *parent = nullptr);

    void setPeriod(int periodMSec);

    void startWithTimeOut(int timeOutMSec);

    //!
    //! will emit \c stopped(false)
    //!
    void stop();

    //!
    //! won't emit \c stopped()
    //!
    void cancel();

signals:
    void triggered();
    void stopped(const bool isDueToTimeOut);

private:
    QTimer *mTimer;
    QElapsedTimer mElapsedTimer;
    int mTimeOut; //(ms)
};

#endif // PERIODICTIMER_H
