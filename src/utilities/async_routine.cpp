#include <QDebug>
#include <QMetaObject>
#include <QTimer>
#include "async_routine.h"
#include "global_constants.h"

namespace {
constexpr bool logVerboseDebugMsg = false;
}

AsyncRoutine::AsyncRoutine(const QString &name)
        : QObject(nullptr)
        , name(name) {
}

void AsyncRoutine::setName(const QString &name_) {
    name = name_;
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

    if constexpr (!buildInReleaseMode && logVerboseDebugMsg) {
        if (!name.isEmpty())
            qDebug().noquote() << QString("routine %1 started").arg(name);
        else
            qDebug().noquote() << "routine" << this << "started";
    }
    ++startedInstances;

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

    if (!steps.at(i).func) {
        qWarning().noquote() << "routine step not defined";
        nextStep();
        return;
    }
    if (steps.at(i).context.isNull()) {
        qWarning().noquote() << QString("context of step %1 has been destroyed").arg(i);
        nextStep();
        return;
    }

    QMetaObject::invokeMethod(steps.at(i).context, steps.at(i).func, Qt::AutoConnection);
}

void AsyncRoutine::finish() {
    if (!isFinished) {
        isFinished = true;
        this->deleteLater();

        --startedInstances;
        if constexpr (!buildInReleaseMode) {
            if constexpr (logVerboseDebugMsg){
                if (!name.isEmpty())
                    qDebug().noquote() << QString("routine %1 finished").arg(name);
                else
                    qDebug().noquote() << "routine" << this << "finished";
            }
            if (logVerboseDebugMsg) {
                qDebug().noquote()
                        << QString("there are %1 unfinished routines").arg(startedInstances);
            }
        }
    }
}

//====

AsyncRoutineWithErrorFlag::AsyncRoutineWithErrorFlag(const QString &name)
    : AsyncRoutine(name) {
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
