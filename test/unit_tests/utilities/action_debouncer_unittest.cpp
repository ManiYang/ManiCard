#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QCoreApplication>
#include <QTest>
#include "utilities/action_debouncer.h"

TEST(ActionDebouncer, NoAct1) {
    bool b = false;
    auto *debouncer = new ActionDebouncer(
            50,
            ActionDebouncer::Option::Delay,
            [&b]() { b = true; }
    );
    QTest::qWait(200);
    debouncer->deleteLater();

    EXPECT_FALSE(b);
}

TEST(ActionDebouncer, NoAct2) {
    bool b = false;
    auto *debouncer = new ActionDebouncer(
            50,
            ActionDebouncer::Option::Ignore,
            [&b]() { b = true; }
    );
    QTest::qWait(200);
    debouncer->deleteLater();

    EXPECT_FALSE(b);
}

TEST(ActionDebouncer, Ignore) {
    int count = 0;
    auto *debouncer = new ActionDebouncer(
            80,
            ActionDebouncer::Option::Ignore,
            [&count]() { count++; }
    );

    bool acted = debouncer->tryAct(); // should act
    EXPECT_TRUE(acted);
    EXPECT_TRUE(count == 1);
    QTest::qWait(30);

    acted = debouncer->tryAct(); // should be ignored
    EXPECT_FALSE(acted);
    QTest::qWait(80);
    EXPECT_TRUE(count == 1);

    acted = debouncer->tryAct(); // should act
    EXPECT_TRUE(acted);
    EXPECT_TRUE(count == 2);

    debouncer->deleteLater();
}

TEST(ActionDebouncer, Delay) {
    int count = 0;
    auto *debouncer = new ActionDebouncer(
            100,
            ActionDebouncer::Option::Delay,
            [&count]() { count++; }
    );

    bool acted = debouncer->tryAct(); // should act
    EXPECT_TRUE(acted);
    EXPECT_TRUE(count == 1);
    EXPECT_FALSE(debouncer->hasDelayed());
    QTest::qWait(30);

    acted = debouncer->tryAct(); // should be delayed
    EXPECT_FALSE(acted);
    QTest::qWait(30);
    EXPECT_TRUE(count == 1);
    EXPECT_TRUE(debouncer->hasDelayed());

    QTest::qWait(70); // after this, the delayed action should have been performed
    EXPECT_TRUE(count == 2);
    EXPECT_FALSE(debouncer->hasDelayed());
    QTest::qWait(20);

    acted = debouncer->tryAct(); // should be delayed
    EXPECT_FALSE(acted);
    QTest::qWait(20);
    EXPECT_TRUE(count == 2);
    EXPECT_TRUE(debouncer->hasDelayed());

    debouncer->deleteLater();
}

TEST(ActionDebouncer, ActNowIgnore) {
    int count = 0;
    auto *debouncer = new ActionDebouncer(
            80,
            ActionDebouncer::Option::Ignore,
            [&count]() { count++; }
    );

    debouncer->tryAct(); // should act
    QTest::qWait(20);
    ASSERT_TRUE(count == 1);

    debouncer->actNow(); // should act
    QTest::qWait(20);
    EXPECT_TRUE(count == 2);

    bool acted = debouncer->tryAct(); // should be ignored
    EXPECT_FALSE(acted);

    debouncer->deleteLater();
}

TEST(ActionDebouncer, ActNowDelay) {
    int count = 0;
    auto *debouncer = new ActionDebouncer(
            80,
            ActionDebouncer::Option::Delay,
            [&count]() { count++; }
    );

    debouncer->tryAct(); // should act
    QTest::qWait(20);
    ASSERT_TRUE(count == 1);

    debouncer->actNow(); // should act
    QTest::qWait(20);
    EXPECT_TRUE(count == 2);
    EXPECT_FALSE(debouncer->hasDelayed());

    bool acted = debouncer->tryAct(); // should be delayed
    EXPECT_FALSE(acted);
    QTest::qWait(20);
    EXPECT_TRUE(debouncer->hasDelayed());

    debouncer->deleteLater();
}

TEST(ActionDebouncer, CancelDelayed) {
    int count = 0;
    auto *debouncer = new ActionDebouncer(
            80,
            ActionDebouncer::Option::Delay,
            [&count]() { count++; }
    );

    debouncer->tryAct(); // should act
    QTest::qWait(20);
    ASSERT_TRUE(count == 1);

    debouncer->tryAct(); // should be delayed
    QTest::qWait(20);
    EXPECT_TRUE(count == 1);
    EXPECT_TRUE(debouncer->hasDelayed());

    debouncer->cancelDelayed(); // delayed action should be canceled
    EXPECT_FALSE(debouncer->hasDelayed());
    QTest::qWait(80);
    EXPECT_TRUE(count == 1);

    debouncer->deleteLater();
}
