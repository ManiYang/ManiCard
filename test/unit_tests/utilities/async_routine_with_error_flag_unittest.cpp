#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QCoreApplication>
#include <QPointer>
#include <QTest>
#include <QThread>
#include "utilities/async_routine.h"

class AsyncRoutineWithErrorFlagTest : public testing::Test
{
protected:
    static void SetUpTestSuite() {
        app = QCoreApplication::instance();
        Q_ASSERT(app != nullptr);
    }

    static void TearDownTestSuite() {}

    inline static QCoreApplication *app = nullptr; // for convenience
};

TEST_F(AsyncRoutineWithErrorFlagTest, NoSkip) {
    QString buffer;

    QPointer<AsyncRoutineWithErrorFlag> routine = new AsyncRoutineWithErrorFlag;
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '1';
    }, app);
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '2';
    }, app);
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '3';
    }, app);
    routine->start();

    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";

    EXPECT_EQ(buffer, QString("123"));
}

TEST_F(AsyncRoutineWithErrorFlagTest, HasSkip) {
    QString buffer;

    QPointer<AsyncRoutineWithErrorFlag> routine = new AsyncRoutineWithErrorFlag;
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '1';
        context.setErrorFlag();
    }, app);
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '2';
    }, app);
    routine->addStep([routine, &buffer]() {
        AsyncRoutineWithErrorFlag::ContinuationContext context(routine);
        buffer += '3';
    }, app);
    routine->start();

    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";

    EXPECT_EQ(buffer, QString("13"));
}
