#ifndef SAVING_DEBOUNCER_H
#define SAVING_DEBOUNCER_H

#include <QTimer>

class SavingDebouncer : public QObject
{
    Q_OBJECT
public:
    explicit SavingDebouncer(const int checkInterval, QObject *parent = nullptr);

    void setUpdated();
    void savingFinished();

    bool isCleared() const;

signals:
    //!
    //! Receiver should save its current state (synchronously or asynchronously). When the
    //! saving finished, call \c savingFinished() .
    //!
    void saveCurrentState();

    void savingScheduled();
    void cleared();

private:
    enum class State {Clear, Pending, Saving};
    State state {State::Clear};

    bool updated;
    QTimer *timer;

    void onTimerTimeout();
};

#endif // SAVING_DEBOUNCER_H
