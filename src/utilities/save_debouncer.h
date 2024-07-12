#ifndef SAVE_DEBOUNCER_H
#define SAVE_DEBOUNCER_H

#include <QTimer>

class SaveDebouncer : public QObject
{
    Q_OBJECT
public:
    explicit SaveDebouncer(const int delayInterval, QObject *parent = nullptr);

    void setUpdated();
    void saveFinished();

    bool isCleared() const;

signals:
    //!
    //! Receiver should save its current state (synchronously or asynchronously). When the
    //! saving finished, call \c saveFinished() .
    //!
    void saveCurrentState();

    void saveScheduled();
    void cleared();

private:
    enum class State {Clear, Pending, Saving};
    State state {State::Clear};

    bool updated;
    QTimer *timer;

    void onTimerTimeout();
};

#endif // SAVE_DEBOUNCER_H
