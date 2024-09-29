#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <unordered_map>
#include <unordered_set>
#include <QDebug>
#include "utilities/variables_update_propagator.h"

struct CustomType
{
    CustomType() {}
    CustomType(const double &v_) : v(v_) {}

    int value() const { return v; }
private:
    int v {0};
};

Q_DECLARE_METATYPE(CustomType);

//====

enum class Var {A, B, X};

inline QString getVarName(const Var var) { // mainly for debugging
    switch(var) {
    case Var::A: return "A";
    case Var::B: return "B";
    case Var::X: return "X";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

inline uint qHash(const Var &var, uint seed) {
   return qHash(static_cast<int>(var), seed);
}

//====

TEST(VarsUpdatePropagator, SimpleGraph_FreeVarsOnly) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addFreeVar(Var::A, 1)
            .addFreeVar(Var::B, 2)
            .addFreeVar(Var::X, 3)
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(Var::A);
    int b = propagator.getValue<int>(Var::B);
    int x = propagator.getValue<int>(Var::X);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(x, 3);

    //
    QHash<Var, QVariant> updatedValues = propagator
            .addUpdate(Var::A, -1)
            .compute();
    EXPECT_TRUE(updatedValues.count() == 1);
    EXPECT_TRUE(updatedValues.contains(Var::A));
    EXPECT_EQ(updatedValues.value(Var::A), QVariant(-1));

    a = propagator.getValue<int>(Var::A);
    b = propagator.getValue<int>(Var::B);
    x = propagator.getValue<int>(Var::X);
    EXPECT_EQ(a, -1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(x, 3);
}

TEST(VarsUpdatePropagator, SimpleGraph1) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                varsAccess.registerOutputVar(Var::X);

                const int a = varsAccess.getInputValue<int>(Var::A);
                const CustomType b = varsAccess.getInputValue<CustomType>(Var::B);

                if (varsAccess.doCompute()) {
                    // compute x
                    Q_ASSERT(!std::isnan(b.value()));
                    QString x = QString("%1, %2").arg(a).arg(b.value());

                    // set result
                    varsAccess.setOutputValue(x);
                }
            })
            .addFreeVar(Var::A, 1)
            .addFreeVar(Var::B, CustomType {2})
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(Var::A);
    CustomType b = propagator.getValue<CustomType>(Var::B);
    QString x = propagator.getValue<QString>(Var::X);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b.value(), 2);
    EXPECT_EQ(x, QString("1, 2"));

    //
    QHash<Var, QVariant> updatedValues = propagator
            .addUpdate(Var::A, -1)
            .compute();
    EXPECT_TRUE(updatedValues.count() == 2);
    EXPECT_TRUE(updatedValues.contains(Var::A));
    EXPECT_TRUE(updatedValues.contains(Var::X));
    EXPECT_EQ(updatedValues.value(Var::A), QVariant(-1));
    EXPECT_EQ(updatedValues.value(Var::X), QVariant(QString("-1, 2")));

    x = propagator.getValue<QString>(Var::X);
    EXPECT_EQ(x, QString("-1, 2"));
}

TEST(VarsUpdatePropagator, SimpleGraph2) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addFreeVar(Var::A, 1)
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);

                varsAccess.registerOutputVar(Var::B);

                if (varsAccess.doCompute()) {
                    const int b = a + 1;
                    varsAccess.setOutputValue(b);
                }
            })
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
                const int b = varsAccess.getInputValue<int>(Var::B);

                varsAccess.registerOutputVar(Var::X);

                if (varsAccess.doCompute()) {
                    const auto x = QString("%1, %2").arg(a).arg(b);
                    varsAccess.setOutputValue(x);
                }
            })
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(Var::A);
    int b = propagator.getValue<int>(Var::B);
    QString x = propagator.getValue<QString>(Var::X);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(x, QString("1, 2"));

    //
    QHash<Var, QVariant> updatedValues = propagator
            .addUpdate(Var::A, 10)
            .compute();
    EXPECT_TRUE(updatedValues.count() == 3);
    EXPECT_TRUE(updatedValues.contains(Var::A));
    EXPECT_TRUE(updatedValues.contains(Var::B));
    EXPECT_TRUE(updatedValues.contains(Var::X));
    EXPECT_EQ(updatedValues.value(Var::A), QVariant(10));
    EXPECT_EQ(updatedValues.value(Var::B), QVariant(11));
    EXPECT_EQ(updatedValues.value(Var::X), QVariant(QString("10, 11")));
}

TEST(VarsUpdatePropagator, InitFail_DoubleRegister1) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
                const CustomType b = varsAccess.getInputValue<CustomType>(Var::B);

                varsAccess.registerOutputVar(Var::X);

                if (varsAccess.doCompute()) {
                    // compute x
                    Q_ASSERT(!std::isnan(b.value()));
                    QString x = QString("%1, %2").arg(a).arg(b.value());

                    // set result
                    varsAccess.setOutputValue(x);
                }
            })
            .addFreeVar(Var::X, 0) // error: already registered
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_DoubleRegister2) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addFreeVar(Var::A, 0)
            .addFreeVar(Var::A, 0) // error: already registered
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_NoInputVar) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                varsAccess.registerOutputVar(Var::X);

                if (varsAccess.doCompute()) {
                    const QString x = "";
                    varsAccess.setOutputValue(x);
                }
            }) // error: no input variable
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_FuncOutputDependsOnSelf) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);

                varsAccess.registerOutputVar(Var::A);

                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(a + 1);
                }
            }) // error: A depends on itself
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_NoOutputVar) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
            }) // error: no output variable
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_OutputValueNotSet) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addFreeVar(Var::A, 0)
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
                varsAccess.registerOutputVar(Var::X);
            }) // error: does not set output value
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_MultipleOutputVars) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addFreeVar(Var::A, 0)
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
                varsAccess.registerOutputVar(Var::B);
                varsAccess.registerOutputVar(Var::X);
                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(QString(""));
                }
            }) // error: register multiple output variables
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_CyclicGraph) {
    VariablesUpdatePropagator<Var> propagator;
    bool ok = propagator
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue<int>(Var::A);
                varsAccess.registerOutputVar(Var::B);
                if (varsAccess.doCompute()) {
                    const int b = a + 1;
                    varsAccess.setOutputValue(b);
                }
            })
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int b = varsAccess.getInputValue<int>(Var::B);
                varsAccess.registerOutputVar(Var::A);
                if (varsAccess.doCompute()) {
                    const int a = b + 1;
                    varsAccess.setOutputValue(a);
                }
            })
            .initialize(); // error: cyclic graph
    EXPECT_FALSE(ok);
}
