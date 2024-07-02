#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <QTest>

template <class T>
bool waitUntilHasValue(std::optional<T> &opt, int timeout = 5000) {
    return QTest::qWaitFor([&opt]() {
        return opt.has_value();
    }, timeout);
}

#endif // TEST_UTIL_H
