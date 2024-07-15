#include <QDebug>
#include <QMetaObject>
#include <QTimer>
#include "async_routine.h"
#include "global_constants.h"

AsyncRoutine::AsyncRoutine()
        : QObject(nullptr) {
}

AsyncRoutine &AsyncRoutine::addStep(std::function<void ()> func, QPointer<QObject> context) {
    Q_ASSERT(!isStarted);
    Q_ASSERT(func);
    Q_ASSERT(!context.isNull());

    steps.push_back(Step {func, context});

    if constexpr (!buildInReleaseMode) {
        QTimer::singleShot(0, this, [this]() {
            if (!isStarted)
                qWarning().noquote() << QString("Did you forget to call AsyncRoutine::start()?");
        });
    }

    return *this;
}

void AsyncRoutine::start() {
    Q_ASSERT(!isStarted);
    isStarted = true;

    if (steps.empty())
        finish();
    else
        QTimer::singleShot(0, this, [this]() { invokeStep(0); });
}

void AsyncRoutine::nextStep() {
    QTimer::singleShot(0, this, [this]() {
        Q_ASSERT(isStarted);
        Q_ASSERT(steps.size() != 0);

        if (isFinished)
            return;

        if (currentStep == steps.size() - 1) {
            finish();
        }
        else {
            ++currentStep;
            invokeStep(currentStep);
        }
    });
}

void AsyncRoutine::skipToFinalStep() {
    QTimer::singleShot(0, this, [this]() {
        Q_ASSERT(isStarted);
        Q_ASSERT(steps.size() != 0);

        if (isFinished)
            return;

        if (currentStep == steps.size() - 1) {
            finish();
        }
        else {
            currentStep = steps.size() - 1;
            invokeStep(currentStep);
        }
    });
}

void AsyncRoutine::invokeStep(const size_t i) {
    Q_ASSERT(i < steps.size());

    if (steps.at(i).context.isNull() || !steps.at(i).func) {
        nextStep();
        return;
    }
    QMetaObject::invokeMethod(steps.at(i).context, steps.at(i).func, Qt::AutoConnection);
}

void AsyncRoutine::finish() {
    if (!isFinished) {
        isFinished = true;
        this->deleteLater();
    }
}

//====

AsyncRoutineWithErrorFlag::AsyncRoutineWithErrorFlag()
    : AsyncRoutine() {
}

// ====

AsyncRoutineWithErrorFlag::ContinuationContext::ContinuationContext(
        AsyncRoutineWithErrorFlag *routine_)
    : routine(routine_) {
}

AsyncRoutineWithErrorFlag::ContinuationContext::~ContinuationContext() {
    if (routine == nullptr)
        return;

    if (routine->errorFlag)
        routine->AsyncRoutine::skipToFinalStep();
    else
        routine->AsyncRoutine::nextStep();
}

void AsyncRoutineWithErrorFlag::ContinuationContext::setErrorFlag() {
    routine->errorFlag = true;
}
