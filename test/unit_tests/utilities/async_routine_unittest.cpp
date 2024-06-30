#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QCoreApplication>
#include <QPointer>
#include <QTest>
#include <QThread>
#include "utilities/async_routine.h"

class AsyncRoutineTest : public testing::Test
{
protected:
    static void SetUpTestSuite() {
        app = QCoreApplication::instance();
        Q_ASSERT(app != nullptr);

        //
        thread1 = new QThread(app);
        objInThread1 = new QObject;
        objInThread1->moveToThread(thread1);

        thread1->start();
        bool ok = QTest::qWaitFor([]() -> bool {
            return thread1->isRunning();
        }, 3000);
        ASSERT_TRUE(ok);
    }

    static void TearDownTestSuite() {
        objInThread1->deleteLater();
        thread1->quit();
        bool ok = QTest::qWaitFor([]() -> bool {
            return thread1->isFinished();
        }, 5000);
        ASSERT_TRUE(ok);
    }

    inline static QCoreApplication *app = nullptr; // for convenience
    inline static QObject *objInThread1 = nullptr;

private:
    inline static QThread *thread1 = nullptr;
};

//!
//! Test that an empty routine can finish successfully, and the routine instance gets deleted.
//!
TEST_F(AsyncRoutineTest, EmptyRoutine) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    routine->start();

    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
}

//!
//! Test that a routine with single synchronous step can finish successfully.
//! Also verify the step is executed exactly once.
//!
TEST_F(AsyncRoutineTest, SingleStep) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    QString buffer;
    routine->addStep([&buffer, routine]() {
        buffer += '*';
        routine->nextStep();
    }, app);
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_EQ(buffer, QString("*"));
}

//!
//! Test that a routine with multiple steps performs the steps in correct order.
//!
TEST_F(AsyncRoutineTest, MultipleSteps) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    QString buffer;
    routine->addStep([&buffer, routine]() {
        buffer += '1';
        routine->nextStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '2';
        routine->nextStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '3';
        routine->nextStep();
    }, app);
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_EQ(buffer, QString("123"));
}

//!
//! Test that skipToFinalStep() works.
//!
TEST_F(AsyncRoutineTest, SkipToFinalStep) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    QString buffer;
    routine->addStep([&buffer, routine]() {
        buffer += '1';
        routine->nextStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '2';
        routine->skipToFinalStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '3';
        routine->nextStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '4';
        routine->nextStep();
    }, app);
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_EQ(buffer, QString("124"));
}

//!
//! Test that skipToFinalStep() can be called at final step.
//!
TEST_F(AsyncRoutineTest, SkipToFinalStepAtFinalStep) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    QString buffer;
    routine->addStep([&buffer, routine]() {
        buffer += '1';
        routine->nextStep();
    }, app);
    routine->addStep([&buffer, routine]() {
        buffer += '2';
        routine->skipToFinalStep();
    }, app);
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_EQ(buffer, QString("12"));
}

//!
//! Test that function is executed in correct thread.
//!
TEST_F(AsyncRoutineTest, CorrectThread) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    QThread *thread = nullptr;
    routine->addStep([&thread, routine]() {
        thread = QThread::currentThread();
        routine->nextStep();
    }, objInThread1);
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_EQ(thread, objInThread1->thread());
}

//!
//! Test that a step won't be perfomed if its context object is deleted.
//!
TEST_F(AsyncRoutineTest, ContextRemoved) {
    QPointer<AsyncRoutine> routine = new AsyncRoutine;
    auto *obj = new QObject(app);
    bool stepPerformed = false;
    routine->addStep([&stepPerformed, routine]() {
        stepPerformed = true;
        routine->nextStep();
    }, obj);
    delete obj;
    routine->start();

    //
    const bool routineDeleted = QTest::qWaitFor([routine]()-> bool {
        return routine.isNull();
    }, 5000);
    ASSERT_TRUE(routineDeleted) << "routine not deleted (within time-out)";
    EXPECT_FALSE(stepPerformed);
}
