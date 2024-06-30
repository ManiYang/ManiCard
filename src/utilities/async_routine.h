#ifndef ASYNC_ROUTINE_H
#define ASYNC_ROUTINE_H

#include <functional>
#include <vector>
#include <QPointer>

//!
//! An utility class for scheduling a sequence of steps (such sequence is called a routine here).
//! Each step consists of a task that can be asynchronous.
//!
//! When a task is completed, call \c nextStep() or \c skipToFinalStep() to let the routine schedule the
//! next step (or last) step in the sequence, or finish if there's no step remaining.
//!
//! Instance of this class auto-deletes itself when finished.
//!
class AsyncRoutine : public QObject
{
    Q_OBJECT

public:
    AsyncRoutine();

    //!
    //! \param func
    //! \param context: the same meaning as in QTimer::singleShot(msec, context, functor)
    //!
    AsyncRoutine &addStep(std::function<void ()> func, QPointer<QObject> context);

    //!
    //! Schedules the first step, or finishes the routine if it is empty.
    //!
    void start();

    //!
    //! Schedules the next step, or finishes the routine if no step remaining.
    //! This method is thread-safe, and must be called only after started.
    //!
    void nextStep();

    //!
    //! Schedules the last step, or finishes the routine if no step remaining.
    //! This method is thread-safe, and must be called only after started.
    //!
    void skipToFinalStep();

private:
    struct Step
    {
        std::function<void ()> func;
        QPointer<QObject> context;
    };

    std::vector<Step> steps;
    size_t currentStep {0};
    bool isStarted {false};
    bool isFinished {false};

    void invokeStep(const size_t i);

    void finish();
};

#endif // ASYNC_ROUTINE_H
