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

enum class FreeVar {A=0, B, C};

inline QString getVarName(const FreeVar var) { // mainly for debugging
    switch(var) {
    case FreeVar::A: return "A";
    case FreeVar::B: return "B";
    case FreeVar::C: return "C";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

inline uint qHash(const FreeVar &var, uint seed) {
   return qHash(static_cast<int>(var), seed);
}

//====

enum class DependentVar {X=0, Y};

inline QString getVarName(const DependentVar var) { // mainly for debugging
    switch(var) {
    case DependentVar::X: return "X";
    case DependentVar::Y: return "Y";
    }
    Q_ASSERT(false); // case not implemented
    return "";
}

inline uint qHash(const DependentVar &var, uint seed) {
   return qHash(static_cast<int>(var), seed);
}

//====

using VarsUpdatePropagator = VariablesUpdatePropagator<FreeVar, DependentVar>;
using VarsAccess = VariablesAccess<FreeVar, DependentVar>;

TEST(VarsUpdatePropagator, SimpleGraph_FreeVarsOnly) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 1)
            .addFreeVar(FreeVar::B, 2)
            .addFreeVar(FreeVar::C, 3)
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(FreeVar::A);
    int b = propagator.getValue<int>(FreeVar::B);
    int c = propagator.getValue<int>(FreeVar::C);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);

    //
    const auto [updatedFreeValues, updatedDependentValues] = propagator
            .addUpdate(FreeVar::A, -1)
            .compute();
    EXPECT_TRUE(updatedFreeValues.count() == 1);
    EXPECT_TRUE(updatedFreeValues.contains(FreeVar::A));
    EXPECT_EQ(updatedFreeValues.value(FreeVar::A), QVariant(-1));

    EXPECT_TRUE(updatedDependentValues.isEmpty());

    a = propagator.getValue<int>(FreeVar::A);
    b = propagator.getValue<int>(FreeVar::B);
    c = propagator.getValue<int>(FreeVar::C);
    EXPECT_EQ(a, -1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(c, 3);
}

TEST(VarsUpdatePropagator, SimpleGraph1) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addDependentVar([](VarsAccess &varsAccess) {
                varsAccess.registerOutputVar(DependentVar::X);

                const int a = varsAccess.getInputValue<int>(FreeVar::A);
                const CustomType b = varsAccess.getInputValue<CustomType>(FreeVar::B);

                if (varsAccess.doCompute()) {
                    // compute x
                    Q_ASSERT(!std::isnan(b.value()));
                    QString x = QString("%1, %2").arg(a).arg(b.value());

                    // set result
                    varsAccess.setOutputValue(x);
                }
            })
            .addFreeVar(FreeVar::A, 1)
            .addFreeVar(FreeVar::B, CustomType {2})
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(FreeVar::A);
    CustomType b = propagator.getValue<CustomType>(FreeVar::B);
    QString x = propagator.getValue<QString>(DependentVar::X);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b.value(), 2);
    EXPECT_EQ(x, QString("1, 2"));

    //
    const auto [updatedFreeValues, updatedDependentValues] = propagator
            .addUpdate(FreeVar::A, -1)
            .compute();
    EXPECT_TRUE(updatedFreeValues.count() == 1);
    EXPECT_TRUE(updatedFreeValues.contains(FreeVar::A));
    EXPECT_EQ(updatedFreeValues.value(FreeVar::A), QVariant(-1));

    EXPECT_TRUE(updatedDependentValues.count() == 1);
    EXPECT_TRUE(updatedDependentValues.contains(DependentVar::X));
    EXPECT_EQ(updatedDependentValues.value(DependentVar::X), QVariant(QString("-1, 2")));

    x = propagator.getValue<QString>(DependentVar::X);
    EXPECT_EQ(x, QString("-1, 2"));
}

TEST(VarsUpdatePropagator, SimpleGraph2) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 1)
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);

                varsAccess.registerOutputVar(DependentVar::X);

                if (varsAccess.doCompute()) {
                    const int x = a + 1;
                    varsAccess.setOutputValue(x);
                }
            })
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);
                const int x = varsAccess.getInputValue<int>(DependentVar::X);

                varsAccess.registerOutputVar(DependentVar::Y);

                if (varsAccess.doCompute()) {
                    const auto y = QString("%1, %2").arg(a).arg(x);
                    varsAccess.setOutputValue(y);
                }
            })
            .initialize();
    EXPECT_TRUE(ok);

    int a = propagator.getValue<int>(FreeVar::A);
    int x = propagator.getValue<int>(DependentVar::X);
    QString y = propagator.getValue<QString>(DependentVar::Y);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, QString("1, 2"));

    //
    const auto [updatedFreeValues, updatedDependentValues] = propagator
            .addUpdate(FreeVar::A, 10)
            .compute();
    EXPECT_TRUE(updatedFreeValues.count() == 1);
    EXPECT_TRUE(updatedFreeValues.contains(FreeVar::A));
    EXPECT_EQ(updatedFreeValues.value(FreeVar::A), QVariant(10));

    EXPECT_TRUE(updatedDependentValues.count() == 2);
    EXPECT_TRUE(updatedDependentValues.contains(DependentVar::X));
    EXPECT_TRUE(updatedDependentValues.contains(DependentVar::Y));
    EXPECT_EQ(updatedDependentValues.value(DependentVar::X), QVariant(11));
    EXPECT_EQ(updatedDependentValues.value(DependentVar::Y), QVariant(QString("10, 11")));
}

TEST(VarsUpdatePropagator, InitFail_DoubleRegister1) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 0)
            .addFreeVar(FreeVar::B, 0)
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);

                varsAccess.registerOutputVar(DependentVar::X);

                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(QString::number(a));
                }
            })
            .addDependentVar([](VarsAccess &varsAccess) {
                const int b = varsAccess.getInputValue<int>(FreeVar::B);

                varsAccess.registerOutputVar(DependentVar::X);

                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(QString::number(b));
                }
            }) // error: X already registered
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_DoubleRegister2) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 0)
            .addFreeVar(FreeVar::A, 0) // error: already registered
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_NoInputVar) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addDependentVar([](VarsAccess &varsAccess) {
                varsAccess.registerOutputVar(DependentVar::X);

                if (varsAccess.doCompute()) {
                    const QString x = "";
                    varsAccess.setOutputValue(x);
                }
            }) // error: no input variable
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_FuncOutputDependsOnSelf) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addDependentVar([](VarsAccess &varsAccess) {
                const int x = varsAccess.getInputValue<int>(DependentVar::X);

                varsAccess.registerOutputVar(DependentVar::X);

                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(x + 1);
                }
            }) // error: A depends on itself
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_NoOutputVar) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);
            }) // error: no output variable
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_OutputValueNotSet) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 0)
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);
                varsAccess.registerOutputVar(DependentVar::X);
            }) // error: does not set output value
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_MultipleOutputVars) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addFreeVar(FreeVar::A, 0)
            .addDependentVar([](VarsAccess &varsAccess) {
                const int a = varsAccess.getInputValue<int>(FreeVar::A);
                varsAccess.registerOutputVar(DependentVar::X);
                varsAccess.registerOutputVar(DependentVar::Y);
                if (varsAccess.doCompute()) {
                    varsAccess.setOutputValue(QString(""));
                }
            }) // error: register multiple output variables
            .initialize();
    EXPECT_FALSE(ok);
}

TEST(VarsUpdatePropagator, InitFail_CyclicGraph) {
    VarsUpdatePropagator propagator(1000, "UnitTest");
    bool ok = propagator
            .addDependentVar([](VarsAccess &varsAccess) {
                const int x = varsAccess.getInputValue<int>(DependentVar::X);
                varsAccess.registerOutputVar(DependentVar::Y);
                if (varsAccess.doCompute()) {
                    const int y = x + 1;
                    varsAccess.setOutputValue(y);
                }
            })
            .addDependentVar([](VarsAccess &varsAccess) {
                const int y = varsAccess.getInputValue<int>(DependentVar::Y);
                varsAccess.registerOutputVar(DependentVar::X);
                if (varsAccess.doCompute()) {
                    const int x = y + 1;
                    varsAccess.setOutputValue(x);
                }
            })
            .initialize(); // error: cyclic graph
    EXPECT_FALSE(ok);
}
