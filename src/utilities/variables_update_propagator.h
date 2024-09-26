#ifndef VARIABLES_UPDATE_PROPAGATOR_H
#define VARIABLES_UPDATE_PROPAGATOR_H

#include <cmath>
#include <functional>
#include <QDebug>
#include <QHash>
#include <QVariant>

/**
 * todo:
 * - add unit test
 * - not every var is cached
 * - incremental update
 */



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

inline uint qHash(Var var, uint seed) {
   return ::qHash(static_cast<int>(var), seed);
}

//====

struct BType
{
    BType();
    BType(const double &v_) : v(v_) {}
    double value() const { return v; }
private:
    double v {std::nan("")};
};

Q_DECLARE_METATYPE(BType);

//========


template <typename VarEnum>
class VariablesUpdatePropagator
{
public:
    struct VariablesAccess
    {
        VariablesAccess(QHash<VarEnum, QVariant> *const valuesPtr_)
            : valuesPtr(valuesPtr_) {}

        QVariant getInputValue(const VarEnum var) {
            if (isPreparingVariables)
                inputVars << var;

            return valuesPtr->value(var);
        }

        void registerOutputVar(const VarEnum var) {
            if (isPreparingVariables)
                outputVars << var;

            registeredOutputVar = var;
        }

        template <typename ValueType>
        void setOutputValue(const ValueType &value) {
            if (!registeredOutputVar.has_value()) {
                qWarning().noquote()
                        << QString("You must call registerOutputVar() before setOutputValue().");
                Q_ASSERT(false);
                return;
            }
            valuesPtr->insert(registeredOutputVar.value(), QVariant::fromValue<ValueType>(value));
            registeredOutputVar = std::nullopt;
        }

        bool getIsPreparingVariables() const { return isPreparingVariables; }

    private:
        friend class VariablesUpdatePropagator;

        QHash<VarEnum, QVariant> *const valuesPtr;

        bool isPreparingVariables {true};

        QSet<VarEnum> inputVars; // used only when `isPreparingVariables` is true
        QSet<VarEnum> outputVars;  // used only when `isPreparingVariables` is true

        std::optional<VarEnum> registeredOutputVar;
    };

public:
    explicit VariablesUpdatePropagator() : variablesAccess(&values) {}

    // ==== initialization ====

    //!
    //! Registers a free variable and sets its initial value.
    //!
    template<typename ValueType>
    VariablesUpdatePropagator &addFreeVar(const VarEnum var, const ValueType &initValue) {
        if (freeVariables.contains(var) || functions.contains(var)) {
            qWarning().noquote()
                    << QString("variable %1 already registered").arg(getVarName(var));
            Q_ASSERT(false);
            return *this;
        }
        freeVariables << var;
        values.insert(var, QVariant::fromValue(initValue));
        return *this;
    }

    using VarComputeFunc = std::function<void (VariablesAccess &varsAccess)>;

    //!
    //! Registers a dependent variable and sets the function that computes it.
    //! Example of `func`:
    //!   void computeX(VariablesUpdatePropagator<Var>::VariablesAccess &varsAccess)
    //!   {
    //!        const int a = varsAccess.getInputValue(Var::A).toInt();
    //!        const TypeOfB b = varsAccess.getInputValue(Var::B).value<TypeOfB>();
    //!        varsAccess.registerOutputVar(Var::X);
    //!        if (varsAccess.getIsPreparingVariables())
    //!            return;
    //!        // compute x
    //!        const TypeOfX x = ...;
    //!        // set result
    //!        varsAccess.setOutputValue(x);
    //!   }
    //!
    VariablesUpdatePropagator &addDependentVar(VarComputeFunc func) {
        variablesAccess.inputVars.clear();
        variablesAccess.outputVars.clear();
        func(variablesAccess);
                // this sets variablesAccess.inputVars & variablesAccess.outputVars

        bool hasError = false;
        if (variablesAccess.outputVars.isEmpty()) {
            qWarning() << QString("function does not register any output variable");
            hasError = true;
        }
        if (!hasError && variablesAccess.outputVars.count() != 1) {
            QStringList varNames;
            for (const VarEnum var: qAsConst(variablesAccess.outputVars))
                varNames << var;
            qWarning() << QString("function registers multiple output variables: %1")
                          .arg(varNames.join(", "));
            hasError = true;
        }

        const auto dependentVar = *variablesAccess.outputVars.constBegin();
        if (!hasError
                && (freeVariables.contains(dependentVar) || functions.contains(dependentVar))) {
            qWarning().noquote()
                    << QString("variable %1 already registered").arg(getVarName(dependentVar));
            hasError = true;
        }
        if (!hasError && variablesAccess.inputVars.contains(dependentVar)) {
            qWarning().noquote()
                    << QString("variable %1 should not depend on itself")
                       .arg(getVarName(dependentVar));
            hasError = true;
        }

        if (hasError) {
            Q_ASSERT(false);
            return *this;
        }

        //
        functions.insert(dependentVar, func);
        dependencies.insert(dependentVar, variablesAccess.inputVars);
        return *this;
    }

    void initialize() {
        // find a topological order


        // compute initial values of dependent variables



        // finish initialization
        variablesAccess.isPreparingVariables = false;
        isInitialized = true;
    }

    // ==== update ====

    template <typename ValueType>
    VariablesUpdatePropagator &addUpdate(const VarEnum var, const ValueType &updatedValue);
            // check isInitialized

    QSet<VarEnum> compute(); // return updated variables
            // check isInitialized

private:
    bool isInitialized {false};

    QSet<VarEnum> freeVariables; // populated during initialization

    QHash<VarEnum, VarComputeFunc> functions;
    QHash<VarEnum, QSet<VarEnum>> dependencies;
            // - keys are the dependent variables
            // - populated during initialization

    QHash<VarEnum, QVariant> values; // keys are all variables

    VariablesAccess variablesAccess;
};

#endif // VARIABLES_UPDATE_PROPAGATOR_H
