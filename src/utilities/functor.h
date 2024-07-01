#ifndef FUNCTOR_H
#define FUNCTOR_H

#include <functional>
#include <QMetaObject>
#include <QPointer>

//!
//! If \e context is not null, will call \c QMetaObject::invokeMethod() with \c Qt::AutoConnection.
//!
inline void invokeAction(QPointer<QObject> context, std::function<void ()> func) {
    Q_ASSERT(func);
    if (!context.isNull())
        QMetaObject::invokeMethod(context, func, Qt::AutoConnection);
}

#endif // FUNCTOR_H
