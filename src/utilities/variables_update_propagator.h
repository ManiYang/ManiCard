#ifndef VARIABLES_UPDATE_PROPAGATOR_H
#define VARIABLES_UPDATE_PROPAGATOR_H

#include <cmath>
#include <functional>
#include <QDebug>
#include <QHash>
#include <QVariant>
#include "utilities/directed_graph.h"
#include "utilities/lists_vectors_util.h"
#include "utilities/maps_util.h"

//!
//! @file
//!
//! The class \c VariablesUpdatePropagator helps build a dependency graph of a set of variables.
//! To use it, you first
//!    - register the free variables (variables that are not depend on other variables) and
//!      their initial values, and
//!    - register dependent variables by defining the functions that compute them (see
//!      \c VariablesUpdatePropagator::addDependentVar() for more details).
//! You then call \c VariablesUpdatePropagator::initialize() to build the graph, after which,
//! when a set of free variables are updated, the class can compute the updated value of the
//! variables that are affected.
//!
//! Variables are identified by the items of the specified enum type \c VarEnum. You must define
//! the following functions for your \c VarEnum:
//!    - <tt>QString getVarName(const VarEnum var);</tt> (mainly for debugging)
//!    - <tt>uint qHash(const VarEnum &var, uint seed);</tt>
//!
//! A custom value type must have a default constructor, and must be declared with
//! \c Q_DECLARE_METATYPE(CustomValueType) .
//!
//! The \c VariablesUpdatePropagator class does not check the types of variable values (it stores
//! the values as \c QVariant objects).
//!
//! The value of every variable is cached.
//!

template <typename VarEnum>
class VariablesUpdatePropagator;

//!
//! See \c VariablesUpdatePropagator::addDependentVar().
//!
template <typename VarEnum>
class VariablesAccess
{
    VariablesAccess(QHash<VarEnum, QVariant> *const valuesPtr_) : valuesPtr(valuesPtr_) {}

public:
    template <typename ValueType>
    ValueType getInputValue(const VarEnum var) {
        if (isPreparingVariables)
            inputVars << var;

        return valuesPtr->value(var).template value<ValueType>();
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
        if (isPreparingVariables)
            return;

        valuesPtr->insert(registeredOutputVar.value(), QVariant::fromValue<ValueType>(value));
        registeredOutputVar = std::nullopt;
        isOutputValueSet = true;
    }

    bool doCompute() const { return !isPreparingVariables; }

private:
    friend class VariablesUpdatePropagator<VarEnum>;

    QHash<VarEnum, QVariant> *const valuesPtr;

    bool isPreparingVariables {true};

    QSet<VarEnum> inputVars; // used only when `isPreparingVariables` is true
    QSet<VarEnum> outputVars;  // used only when `isPreparingVariables` is true

    bool isOutputValueSet {false}; // used only when computing initial value

    std::optional<VarEnum> registeredOutputVar;
};

//====

template <typename VarEnum>
class VariablesUpdatePropagator
{
public:


public:
    explicit VariablesUpdatePropagator() {}

    // ==== initialization ====

    //!
    //! Registers a free variable and sets its initial value.
    //!
    template<typename ValueType>
    VariablesUpdatePropagator &addFreeVar(const VarEnum var, const ValueType &initValue) {
        if (isInitialized) {
            qWarning().noquote() << "VariablesUpdatePropagator already initialized";
            Q_ASSERT(false);
            return *this;
        }

        if (freeVariables.contains(var) || functions.contains(var)) {
            qWarning().noquote()
                    << QString("variable %1 already registered").arg(getVarName(var));
            hasInitializationError = true;
            return *this;
        }

        freeVariables << var;
        values.insert(var, QVariant::fromValue(initValue));
        return *this;
    }

    using VarComputeFunc = std::function<void (VariablesAccess<VarEnum> &varsAccess)>;

    //!
    //! Registers a dependent variable and sets the "computation function" that computes it.
    //!
    //! - The computation function should be a function whose job is to compute the value of the
    //!   output variable from the values of input variables.
    //!
    //! - The computation function must have at least one input variable, and exactly one output
    //!   variable. (But the type of output variable can be a structure that wraps multiple items,
    //!   like \c std::pair or \c struct.)
    //!
    //! - In the definition of the computation function, use the \e varsAccess parameter to
    //!   get/set values of variables. The output variable must be registered before it can be set
    //!   with a value.
    //!
    //! Example of computation function:
    //! \code{.cpp}
    //!     void computeX(VariablesAccess<Var> &varsAccess) {
    //!          // 1. register output variable & get input variables
    //!          varsAccess.registerOutputVar(Var::X);
    //!
    //!          const int a = varsAccess.getInputValue<int>(Var::A);
    //!          const double b = varsAccess.getInputValue<double>(Var::B);
    //!
    //!          if (varsAccess.doCompute()) {
    //!              // 2. compute x
    //!              const TypeOfX x = ...;
    //!
    //!              // 3. set result
    //!              varsAccess.setOutputValue(x);
    //!          }
    //!     }
    //! \endcode
    //!
    VariablesUpdatePropagator &addDependentVar(VarComputeFunc func) {
        if (isInitialized) {
            qWarning().noquote() << "VariablesUpdatePropagator already initialized";
            Q_ASSERT(false);
            return *this;
        }

        //
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
                varNames << getVarName(var);
            qWarning() << QString("function registers multiple output variables: %1")
                          .arg(varNames.join(", "));
            hasError = true;
        }

        const auto dependentVar = *variablesAccess.outputVars.constBegin();
        if (!hasError && variablesAccess.inputVars.isEmpty()) {
            qWarning()
                    << QString("function for variable %1 does not register any input variable")
                       .arg(getVarName(dependentVar));
            hasError = true;
        }
        if (!hasError
                && (freeVariables.contains(dependentVar) || functions.contains(dependentVar))) {
            qWarning().noquote()
                    << QString("variable %1 already registered").arg(getVarName(dependentVar));
            hasError = true;
        }
        if (!hasError && variablesAccess.inputVars.contains(dependentVar)) {
            qWarning().noquote()
                    << QString("variable %1 depends on itself")
                       .arg(getVarName(dependentVar));
            hasError = true;
        }

        if (hasError) {
            hasInitializationError = true;
            return *this;
        }

        //
        allInputVars |= variablesAccess.inputVars;
        functions.insert(dependentVar, func);
        dependencies.insert(dependentVar, variablesAccess.inputVars);
        return *this;
    }

    //!
    //! \return successful?
    //!
    bool initialize() {
        if (isInitialized) {
            qWarning().noquote() << "VariablesUpdatePropagator already initialized";
            Q_ASSERT(false);
            return isInitializationSuccessful();
        }

        //
        const QSet<VarEnum> allRegisteredVars = keySet(dependencies) | freeVariables;

        if (!allRegisteredVars.contains(allInputVars)) {
            const auto missingVars = allInputVars - allRegisteredVars;
            QStringList missingVarNames;
            for (const VarEnum var: missingVars)
                missingVarNames << getVarName(var);
            qWarning().noquote()
                    << QString("the following variables are not registered "
                               "but are used as dependencies: %1")
                       .arg(missingVarNames.join(", "));
            hasInitializationError = true;
        }

        // create graph
        for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it) {
            VarEnum dependentVar = it.key();
            const QSet<VarEnum> &varSet = it.value();
            for (const VarEnum var: varSet)
                graph.addEdge(var, dependentVar);
        }

        // find a topological order
        {
            const std::vector<VarEnum> sortedVars = graph.topologicalOrder(); // empty if failed
            if (!dependencies.isEmpty() && sortedVars.empty()) {
                qWarning().noquote() << "the dependency graph is cyclic";
                hasInitializationError = true;
            }

            dependentVarsInTopologicalOrder.reserve(sortedVars.size());
            for (const VarEnum var: sortedVars) {
                if (dependencies.contains(var))
                    dependentVarsInTopologicalOrder << var;
            }
        }

        // compute initial values of dependent variables
        variablesAccess.isPreparingVariables = false;

        for (const VarEnum var: dependentVarsInTopologicalOrder) {
            variablesAccess.isOutputValueSet = false;
            functions[var](variablesAccess);

            if (!variablesAccess.isOutputValueSet) {
                qWarning()
                        << QString("function for variable %1 does not set output value")
                           .arg(getVarName(var));
                hasInitializationError = true;
            }
        }

        // finish initialization
        isInitialized = true;

        if (hasInitializationError) {
            Q_ASSERT(false);
        }
        else {
            // check that every variable has been initialized
            Q_ASSERT(keySet(values) == allRegisteredVars);
        }

        return isInitializationSuccessful();
    }

    bool isInitializationSuccessful() const {
        return isInitialized && !hasInitializationError;
    }

    // ==== get ====
    bool hasVar(const VarEnum var) const {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        return values.contains(var);
    }

    //!
    //! \return a default-constructed value if the stored \c QVariant cannot be converted to
    //!         \e ValueType
    //!
    template <typename ValueType>
    ValueType getValue(const VarEnum var) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            qWarning().noquote() << "initialization was not successful";
            return ValueType();
        }

        Q_ASSERT(values.contains(var));
        if (!values[var].template canConvert<ValueType>())
            qWarning().noquote() << "value cannot be converted to required value type";
        return values[var].template value<ValueType>();
    }

    QVariant getValueAsVariant(const VarEnum var) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            qWarning().noquote() << "initialization was not successful";
            return {};
        }

        Q_ASSERT(values.contains(var));
        return values[var];
    }

    // ==== update ====

    //!
    //! \param var: must be a free variable
    //! \param updatedValue
    //!
    template <typename ValueType>
    VariablesUpdatePropagator &addUpdate(const VarEnum freeVar, const ValueType &updatedValue) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            qWarning().noquote() << "initialization was not successful";
            return *this;
        }

        if (!freeVariables.contains(freeVar)) {
            qWarning().noquote()
                    << QString("%1 is not a free variable").arg(getVarName(freeVar));
            Q_ASSERT(false);
            return *this;
        }

        updatedFreeVarToValue.insert(freeVar, QVariant::fromValue<ValueType>(updatedValue));
        return *this;
    }

    //!
    //! \return updated variables and values (You can also use \c getValue() or
    //!         \c getValueAsVariant() to get updated values.)
    //!
    QHash<VarEnum, QVariant> compute() {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            qWarning().noquote() << "initialization was not successful";
            return {};
        }

        const QSet<VarEnum> updatedFreeVarsSet = keySet(updatedFreeVarToValue);
        if (!updatedFreeVarsToAffectedDependentVars.contains(updatedFreeVarsSet)) {
            // find the affected dependent vars
            QSet<VarEnum> affectedVarsSet;
            for (const auto var: updatedFreeVarsSet) {
                std::vector<VarEnum> visitedVars = graph.breadthFirstSearch(var);
                QSet<VarEnum> visitedDependentVars
                        = QSet<VarEnum>(visitedVars.cbegin(), visitedVars.cend())
                          & keySet(dependencies);

                affectedVarsSet |= visitedDependentVars;
            }

            const QVector<VarEnum> affectedVarsSorted = sortByOrdering(
                    affectedVarsSet, dependentVarsInTopologicalOrder, false);

            updatedFreeVarsToAffectedDependentVars.insert(
                    updatedFreeVarsSet, affectedVarsSorted);
        }

        // compute
        QHash<VarEnum, QVariant> updatedValues; // for return

        for (auto it = updatedFreeVarToValue.constBegin();
                it != updatedFreeVarToValue.constEnd(); ++it) {
            values.insert(it.key(), it.value());
            updatedValues.insert(it.key(), it.value());
        }

        const QVector<VarEnum> varsToCompute
                = updatedFreeVarsToAffectedDependentVars.value(updatedFreeVarsSet);
        for (const auto var: varsToCompute) {
            if (!functions.contains(var)) {
                qWarning().noquote() << QString("`functions[%1]` not found").arg(getVarName(var));
                continue;
            }
            functions.value(var)(variablesAccess);
            updatedValues.insert(var, values.value(var));
        }

        //
        updatedFreeVarToValue.clear();
        return updatedValues;
    }

private:
    bool isInitialized {false}; // has initialize() been called?
    bool hasInitializationError {false};

    QSet<VarEnum> freeVariables; // populated during initialization

    QHash<VarEnum, VarComputeFunc> functions;
    QHash<VarEnum, QSet<VarEnum>> dependencies;
            // - keys are the dependent variables
            // - populated during initialization

    QSet<VarEnum> allInputVars; // populated during initialization

    DirectedGraphWithVertexEnum<VarEnum> graph;
            // - an edge from A to B means that B depends on A
            // - set in initialize()
    QVector<VarEnum> dependentVarsInTopologicalOrder; // set in initialize()

    QHash<VarEnum, QVariant> values; // keys are all variables
    VariablesAccess<VarEnum> variablesAccess {&values};
    QHash<VarEnum, QVariant> updatedFreeVarToValue; // populated in addUpdate(), cleared in compute()

    template <typename ValueType>
    struct HashMapWithVarSetAsKey
    {
        // purpose of this struct is to avoid the need of defining `qHash(QSet<VarEnum> s)`

        bool contains(const QSet<VarEnum> &varSet) const {
            return map.contains(toIntSet(varSet));
        }
        void insert(const QSet<VarEnum> &varSet, const ValueType &value) {
            map.insert(toIntSet(varSet), value);
        }
        ValueType value(const QSet<VarEnum> &varSet) const {
            return map.value(toIntSet(varSet));
        }
    private:
        QHash<QSet<int>, ValueType> map;

        static QSet<int> toIntSet(const QSet<VarEnum> &varSet) {
            QSet<int> intSet;
            for (const VarEnum var: varSet)
                intSet << static_cast<int>(var);
            return intSet;
        }
    };
    HashMapWithVarSetAsKey<QVector<VarEnum>> updatedFreeVarsToAffectedDependentVars;
            // - values are in topological order
            // - populated and used in compute(), as a cache
};

#endif // VARIABLES_UPDATE_PROPAGATOR_H
