#ifndef ASYNC_ROUTINE_H
#define ASYNC_ROUTINE_H

#include <functional>
#include <vector>
#include <QPointer>

//!
//! An utility class for defining and running a sequence of steps (such sequence is called a routine
//! here). Each step consists of a task that can be asynchronous.
//!
//! When a task is completed, call \c nextStep() or \c skipToFinalStep() to let the routine continue
//! to the next (or last) step in the sequence, or finish if there's no step remaining.
//!
//! Instance of this class auto-deletes itself when finished.
//!
class AsyncRoutine : public QObject
{
    Q_OBJECT
public:
    AsyncRoutine(const QString &name = "");

    void setName(const QString &name);

    //!
    //! \e Func will be invoked in the thread of \e context. If \e context is destroyed before
    //! the step starts, \e func will not be invoked (but you should take care that objects used
    //! in \e func are still alive when it is invoked).
    //!
    AsyncRoutine &addStep(std::function<void ()> func, QPointer<QObject> context);

    //!
    //! Schedules the first step, or finishes the routine if it is empty.
    //!
    void start();

    //!
    //! Schedules the next step, or finishes the routine if no step remaining.
    //! This method is thread-safe, and can only be called at the very end of a step.
    //!
    void nextStep();

    //!
    //! Schedules the last step, or finishes the routine if no step remaining.
    //! This method is thread-safe, and can only be called at the very end of a step.
    //!
    void skipToFinalStep();

private:
    inline static int startedInstances {0};

    struct Step
    {
        std::function<void ()> func;
        QPointer<QObject> context;
    };

    QString name;
    std::vector<Step> steps;
    size_t currentStep {0};
    bool isStarted {false};
    bool isFinished {false};

    void invokeStep(const size_t i);

    void finish();
};


class AsyncRoutineWithErrorFlag : public AsyncRoutine
{
    Q_OBJECT
public:
    AsyncRoutineWithErrorFlag(const QString &name = "");

    bool errorFlag {false};

    //!
    //! Destructor of this class calls routin->nextStep() [routin->skipToFinalStep()] if the
    //! the value of routin->errorFlag is false [true].
    //!
    struct ContinuationContext
    {
        ContinuationContext(AsyncRoutineWithErrorFlag *routine_);
        ~ContinuationContext();
        void setErrorFlag(); // sets routine->errorFlag to true
    private:
        AsyncRoutineWithErrorFlag *routine;
    };
};

#endif // ASYNC_ROUTINE_H
