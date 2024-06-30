#include <QMetaObject>
#include <QTimer>
#include "async_routine.h"

AsyncRoutine::AsyncRoutine() : QObject(nullptr) {}

AsyncRoutine &AsyncRoutine::addStep(std::function<void ()> func, QPointer<QObject> context) {
    Q_ASSERT(!isStarted);
    Q_ASSERT(func);
    Q_ASSERT(!context.isNull());

    steps.push_back(Step {func, context});
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

void AsyncRoutine::nextStep()
{
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

void AsyncRoutine::skipToFinalStep()
{
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
