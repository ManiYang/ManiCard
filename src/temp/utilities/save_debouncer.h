#ifndef SAVE_DEBOUNCER_H
#define SAVE_DEBOUNCER_H

#include <QTimer>

//!
//! This class can be used when we have an editor that user can edit (e.g., a QTextEdit),
//! and while user is editing, updates are saved (e.g., to a DB) but not too frequently.
//!
class SaveDebouncer : public QObject
{
    Q_OBJECT
public:
    explicit SaveDebouncer(const int delayInterval, QObject *parent = nullptr);

    //!
    //! Call this whenever an update is made.
    //!
    void setUpdated();

    //!
    //! Call this when saving finished (not necessarily successful).
    //! See signal saveCurrentState().
    //!
    void saveFinished();

    //!
    //! Call this when an immediate save is needed.
    //!
    void saveNow();

    //!
    //! \return true if the state is "cleared" (i.e., saveFinished() has been called for the
    //!         lastest update)
    //!
    bool isCleared() const;

signals:
    //!
    //! Receiver should save its current state (synchronously or asynchronously). When the
    //! saving finished (not necessarily successful), call \c saveFinished() .
    //!
    void saveCurrentState();

    // signals about state change
    void saveScheduled(); // out of "cleared" state
    void cleared(); // into "cleared" state

private:
    enum class State {Clear, Pending, Saving};
    State state {State::Clear};

    bool updated;
    QTimer *timer;
    int delayInterval;

    void onTimerTimeout();
    void enterSavingState();
};

#endif // SAVE_DEBOUNCER_H
