#include "action_debouncer.h"

ActionDebouncer::ActionDebouncer(
        const int minimumSeperationMsec, const Option option_,
        std::function<void ()> action_, QObject *parent)
            : QObject(parent)
            , action(action_)
            , option(option_)
            , timer(new QTimer(this)) {
    Q_ASSERT(action_);

    timer->setSingleShot(true);
    timer->setInterval(minimumSeperationMsec);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (delayed)
            tryAct();
    });
}

bool ActionDebouncer::tryAct() {
    if (timer->isActive()) {
        if (option == Option::Delay)
            delayed = true;
        return false;
    }
    else {
        action();
        delayed = false;
        timer->start();
        return true;
    }
}

void ActionDebouncer::actNow() {
    timer->stop();
    tryAct();
}

void ActionDebouncer::cancelDelayed() {
    delayed = false;
}

bool ActionDebouncer::hasDelayed() const {
    return delayed;
}
