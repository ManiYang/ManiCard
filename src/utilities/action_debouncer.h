#ifndef ACTIONDEBOUNCER_H
#define ACTIONDEBOUNCER_H

#include <functional>
#include <QObject>
#include <QTimer>

class ActionDebouncer : public QObject
{
    Q_OBJECT
public:
    enum class Option {Delay, Ignore};

    //!
    //! The function \e action will be invoked in the context of \c this.
    //!
    explicit ActionDebouncer(
            const int minimumSeperationMsec, const Option option,
            std::function<void ()> action, QObject *parent = nullptr);

    //!
    //! The action won't be performed if the last time it executed is within
    //! \e minimumSeperationMsec, in which case it is delay or ignored, depending on the value
    //! of \e option.
    //! Otherwise, the action is performed.
    //! \return true if the action is performed
    //!
    bool tryAct();

    //!
    //! Performs the action, disregarding the minimum separation. If there were delayed
    //! (scheduled) call to \e action, it is canceled first.
    //!
    void actNow();

    //!
    //! Cancel the current delayed (scheduled) call to \e action. (Meaningful only if
    //! \e option is \c Delay)
    //!
    void cancelDelayed();

private:
    const std::function<void ()> action;
    const Option option;
    bool delayed {false};
    QTimer *timer;
};

#endif // ACTIONDEBOUNCER_H
