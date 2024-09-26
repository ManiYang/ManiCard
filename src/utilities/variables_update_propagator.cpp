#include "variables_update_propagator.h"

void example() {
    VariablesUpdatePropagator<Var> propagator;

    propagator
            .addFreeVar(Var::A, 123)
            .addFreeVar(Var::B, BType{4.56})
            .addDependentVar([](VariablesAccess<Var> &varsAccess) {
                const int a = varsAccess.getInputValue(Var::A).toInt();
                const BType b = varsAccess.getInputValue(Var::B).value<BType>();

                varsAccess.registerOutputVar(Var::X);
                if (varsAccess.getIsPreparingVariables())
                    return;

                // compute x
                Q_ASSERT(!std::isnan(b.value()));
                QString x = QString("%1, %2").arg(a).arg(b.value());

                // set result
                varsAccess.setOutputValue(x);
            })
            .initialize();
}
