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


// FreeVarEnum, DependentVarEnum must have \c int as the underlying type, and the items cannot
// have negative value.


template <typename FreeVarEnum, typename DependentVarEnum>
class VariablesUpdatePropagator;

template <typename FreeVarEnum, typename DependentVarEnum>
class VarIdHelper
{
    VarIdHelper(const int maxOfFreeVarItemValue)
        : dependentVarIdMin(maxOfFreeVarItemValue + 1) {}

public:
    int toVarId(const FreeVarEnum freeVar) const {
        return static_cast<int>(freeVar);
    }

    int toVarId(const DependentVarEnum dependentVar) const {
        return dependentVarIdMin + static_cast<int>(dependentVar);
    }

    bool isFreeVar(const int varId) const {
        return varId < dependentVarIdMin;
    }

    FreeVarEnum toFreeVar (const int varId) const {
        Q_ASSERT(isFreeVar(varId));
        return static_cast<FreeVarEnum>(varId);
    }

    DependentVarEnum toDependentVar (const int varId) const {
        Q_ASSERT(!isFreeVar(varId));
        return static_cast<DependentVarEnum>(varId - dependentVarIdMin);
    }

private:
    friend class VariablesUpdatePropagator<FreeVarEnum, DependentVarEnum>;
    const int dependentVarIdMin;
};

//!
//! Ths class is used in \c VariablesUpdatePropagator::addDependentVar().
//!
template <typename FreeVarEnum, typename DependentVarEnum>
class VariablesAccess
{
    VariablesAccess(
            const QString &propagatorName_,
            QHash<int, QVariant> *const valuesPtr_,
            VarIdHelper<FreeVarEnum, DependentVarEnum> *varIdHelper_)
        : propagatorName(propagatorName_), valuesPtr(valuesPtr_), varIdHelper(varIdHelper_) {}

public:
    template <typename ValueType>
    ValueType getInputValue(const FreeVarEnum freeVar) {
        const int varId = varIdHelper->toVarId(freeVar);

        if (isPreparingVariables)
            inputVars << varId;

        return valuesPtr->value(varId).template value<ValueType>();
    }

    template <typename ValueType>
    ValueType getInputValue(const DependentVarEnum dependentVar) {
        const int varId = varIdHelper->toVarId(dependentVar);

        if (isPreparingVariables)
            inputVars << varId;

        return valuesPtr->value(varId).template value<ValueType>();
    }

    void registerOutputVar(const DependentVarEnum dependentVar) {
        const int varId = varIdHelper->toVarId(dependentVar);

        if (isPreparingVariables)
            outputVars << varId;

        registeredOutputVarId = varId;
    }

    template <typename ValueType>
    void setOutputValue(const ValueType &value) {
        if (!registeredOutputVarId.has_value()) {
            qWarning().noquote()
                    << QString("%1: You must call registerOutputVar() before setOutputValue().")
                       .arg(propagatorName);
            Q_ASSERT(false);
            return;
        }

        if (isPreparingVariables)
            return;

        valuesPtr->insert(registeredOutputVarId.value(), QVariant::fromValue<ValueType>(value));
        registeredOutputVarId = std::nullopt;
        isOutputValueSet = true;
    }

    bool doCompute() const { return !isPreparingVariables; }

private:
    friend class VariablesUpdatePropagator<FreeVarEnum, DependentVarEnum>;

    const QString propagatorName;
    QHash<int, QVariant> *const valuesPtr; // key: var ID
    VarIdHelper<FreeVarEnum, DependentVarEnum> *const varIdHelper;

    bool isPreparingVariables {true};

    QSet<int> inputVars; // used only when `isPreparingVariables` is true
    QSet<int> outputVars;  // used only when `isPreparingVariables` is true

    bool isOutputValueSet {false}; // used only when computing initial value

    std::optional<int> registeredOutputVarId;
};

//====

template <typename FreeVarEnum, typename DependentVarEnum>
class VariablesUpdatePropagator
{
public:
    //!
    //! \param maxOfFreeVarItemValue: a number that is >= the largest \c int value of the
    //!                               FreeVarEnum items
    //! \param name: mainly for debugging
    //!
    explicit VariablesUpdatePropagator(const int maxOfFreeVarItemValue, const QString &name)
        : name(name)
        , varIdHelper(maxOfFreeVarItemValue)
        , variablesAccess(name, &values, &varIdHelper)
    {}

    // ==== initialization ====

    //!
    //! Registers a free variable and sets its initial value.
    //!
    VariablesUpdatePropagator &addFreeVar(const FreeVarEnum freeVar, const QVariant &initValue) {
        if (isInitialized) {
            logWarning("VariablesUpdatePropagator already initialized");
            Q_ASSERT(false);
            return *this;
        }

        const int varId = varIdHelper.toVarId(freeVar);

        if (freeVariables.contains(varId) || functions.contains(varId)) {
            logWarning(QString("variable %1 already registered").arg(getVarName(freeVar)));
            hasInitializationError = true;
            return *this;
        }

        freeVariables << varId;
        values.insert(varId, initValue);
        return *this;
    }

    //!
    //! Convenience method.
    //!
    template<typename ValueType>
    VariablesUpdatePropagator &addFreeVar(const FreeVarEnum freeVar, const ValueType &initValue) {
        return addFreeVar(freeVar, QVariant::fromValue<ValueType>(initValue));
    }

    using VarComputeFunc
        = std::function<void (VariablesAccess<FreeVarEnum, DependentVarEnum> &varsAccess)>;

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
            logWarning("VariablesUpdatePropagator already initialized");
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
            logWarning("function does not register any output variable");
            hasError = true;
        }
        if (!hasError && variablesAccess.outputVars.count() != 1) {
            QStringList varNames;
            for (const int varId: qAsConst(variablesAccess.outputVars))
                varNames << getVariableName(varId);
            logWarning(
                    QString("function registers multiple output variables: %1")
                    .arg(varNames.join(", ")));
            hasError = true;
        }

        const int dependentVarId = *variablesAccess.outputVars.constBegin();
        if (!hasError && variablesAccess.inputVars.isEmpty()) {
            logWarning(
                    QString("function for variable %1 does not register any input variable")
                    .arg(getVariableName(dependentVarId)));
            hasError = true;
        }
        if (!hasError
                && (freeVariables.contains(dependentVarId) || functions.contains(dependentVarId))) {
            logWarning(
                    QString("variable %1 already registered").arg(getVariableName(dependentVarId)));
            hasError = true;
        }
        if (!hasError && variablesAccess.inputVars.contains(dependentVarId)) {
            logWarning(
                    QString("variable %1 depends on itself").arg(getVariableName(dependentVarId)));
            hasError = true;
        }

        if (hasError) {
            hasInitializationError = true;
            return *this;
        }

        //
        allInputVars |= variablesAccess.inputVars;
        functions.insert(dependentVarId, func);
        dependencies.insert(dependentVarId, variablesAccess.inputVars);
        return *this;
    }

    //!
    //! \return successful?
    //!
    bool initialize() {
        if (isInitialized) {
            logWarning("VariablesUpdatePropagator already initialized");
            Q_ASSERT(false);
            return isInitializationSuccessful();
        }

        //
        const QSet<int> allRegisteredVars = keySet(dependencies) | freeVariables;

        if (!allRegisteredVars.contains(allInputVars)) {
            const auto missingVars = allInputVars - allRegisteredVars;
            QStringList missingVarNames;
            for (const int varId: missingVars)
                missingVarNames << getVariableName(varId);
            logWarning(
                    QString("the following variables are not registered "
                            "but are used as dependencies: %1")
                    .arg(missingVarNames.join(", ")));
            hasInitializationError = true;
        }

        // create graph
        for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it) {
            const int dependentVar = it.key();
            const QSet<int> &varSet = it.value();
            for (const int var: varSet)
                graph.addEdge(var, dependentVar);
        }

        // find a topological order
        {
            const std::vector<int> sortedVars = graph.topologicalOrder(); // empty if graph is cyclic
            if (!dependencies.isEmpty() && sortedVars.empty()) {
                logWarning("the dependency graph is cyclic");
                hasInitializationError = true;
            }

            dependentVarsInTopologicalOrder.reserve(sortedVars.size());
            for (const int var: sortedVars) {
                if (dependencies.contains(var))
                    dependentVarsInTopologicalOrder << var;
            }
        }

        // compute initial values of dependent variables
        variablesAccess.isPreparingVariables = false;

        for (const int varId: qAsConst(dependentVarsInTopologicalOrder)) {
            variablesAccess.isOutputValueSet = false;
            functions[varId](variablesAccess);

            if (!variablesAccess.isOutputValueSet) {
                logWarning(
                        QString("function for variable %1 does not set output value")
                        .arg(getVariableName(varId)));
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
    bool hasVar(const FreeVarEnum freeVar) const {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        const int varId = varIdHelper.toVarId(freeVar);
        return values.contains(varId);
    }

    bool hasVar(const DependentVarEnum dependentVar) const {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        const int varId = varIdHelper.toVarId(dependentVar);
        return values.contains(varId);
    }

    //!
    //! \return a default-constructed value if the stored \c QVariant cannot be converted to
    //!         \e ValueType
    //!
    template <typename ValueType>
    ValueType getValue(const FreeVarEnum freeVar) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return ValueType();
        }

        const int varId = varIdHelper.toVarId(freeVar);
        Q_ASSERT(values.contains(varId));
        if (!values[varId].template canConvert<ValueType>())
            logWarning("value cannot be converted to required value type");
        return values[varId].template value<ValueType>();
    }

    //!
    //! \return a default-constructed value if the stored \c QVariant cannot be converted to
    //!         \e ValueType
    //!
    template <typename ValueType>
    ValueType getValue(const DependentVarEnum dependentVar) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return ValueType();
        }

        const int varId = varIdHelper.toVarId(dependentVar);
        Q_ASSERT(values.contains(varId));
        if (!values[varId].template canConvert<ValueType>())
            logWarning("value cannot be converted to required value type");
        return values[varId].template value<ValueType>();
    }

    QVariant getValueAsVariant(const FreeVarEnum freeVar) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return {};
        }

        const int varId = varIdHelper.toVarId(freeVar);
        Q_ASSERT(values.contains(varId));
        return values[varId];
    }

    QVariant getValueAsVariant(const DependentVarEnum dependentVar) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return {};
        }

        const int varId = varIdHelper.toVarId(dependentVar);
        Q_ASSERT(values.contains(varId));
        return values[varId];
    }

    // ==== update ====

    VariablesUpdatePropagator &addUpdate(
            const FreeVarEnum freeVar, const QVariant &updatedValue) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return *this;
        }

        const int varId = varIdHelper.toVarId(freeVar);
        updatedFreeVarToValue.insert(varId, updatedValue);
        return *this;
    }

    //!
    //! Convenience method.
    //!
    template <typename ValueType>
    VariablesUpdatePropagator &addUpdate(
            const FreeVarEnum freeVar, const ValueType &updatedValue) {
        return addUpdate(freeVar, QVariant::fromValue<ValueType>(updatedValue));
    }

    using FreeVarsUpdatedValues = QHash<FreeVarEnum, QVariant>;
    using DependentVarsUpdatedValues = QHash<DependentVarEnum, QVariant>;

    //!
    //! \return updated variables and values (You can also use \c getValue() or
    //!         \c getValueAsVariant() to get updated values.)
    //!
    std::pair<FreeVarsUpdatedValues, DependentVarsUpdatedValues> compute() {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (hasInitializationError) {
            logWarning("initialization was not successful");
            return {};
        }

        const QSet<int> updatedFreeVarsSet = keySet(updatedFreeVarToValue);
        if (!updatedFreeVarsToAffectedDependentVars.contains(updatedFreeVarsSet)) {
            // find the affected dependent vars
            QSet<int> affectedVarsSet;
            for (const auto var: updatedFreeVarsSet) {
                std::vector<int> visitedVars = graph.breadthFirstSearch(var);
                QSet<int> visitedDependentVars
                        = QSet<int>(visitedVars.cbegin(), visitedVars.cend())
                          & keySet(dependencies);

                affectedVarsSet |= visitedDependentVars;
            }

            const QVector<int> affectedVarsSorted = sortByOrdering(
                    affectedVarsSet, dependentVarsInTopologicalOrder, false);

            updatedFreeVarsToAffectedDependentVars.insert(
                    updatedFreeVarsSet, affectedVarsSorted);
        }

        // compute
        FreeVarsUpdatedValues freeVarsUpdatedValues; // for return
        DependentVarsUpdatedValues dependentVarsUpdatedValues; // for return

        for (auto it = updatedFreeVarToValue.constBegin();
                it != updatedFreeVarToValue.constEnd(); ++it) {
            values.insert(it.key(), it.value());
            freeVarsUpdatedValues.insert(varIdHelper.toFreeVar(it.key()), it.value());
        }

        const QVector<int> varsToCompute
                = updatedFreeVarsToAffectedDependentVars.value(updatedFreeVarsSet);
        for (const auto var: varsToCompute) {
            if (!functions.contains(var)) {
                logWarning(QString("`functions[%1]` not found").arg(getVariableName(var)));
                continue;
            }
            functions.value(var)(variablesAccess);
            dependentVarsUpdatedValues.insert(
                    varIdHelper.toDependentVar(var), values.value(var));
        }

        //
        updatedFreeVarToValue.clear();
        return {freeVarsUpdatedValues, dependentVarsUpdatedValues};
    }

private:
    const QString name;
    VarIdHelper<FreeVarEnum, DependentVarEnum> varIdHelper;

    bool isInitialized {false}; // has initialize() been called?
    bool hasInitializationError {false};

    QSet<int> freeVariables; // populated during initialization

    QHash<int, VarComputeFunc> functions;
    QHash<int, QSet<int>> dependencies;
            // - keys are ID's of the dependent variables
            // - populated during initialization

    QSet<int> allInputVars; // populated during initialization

    DirectedGraph graph;
            // - an edge from A to B means that B depends on A
            // - set in initialize()
    QVector<int> dependentVarsInTopologicalOrder; // set in initialize()

    QHash<int, QVariant> values; // keys are all variables
    VariablesAccess<FreeVarEnum, DependentVarEnum> variablesAccess;
    QHash<int, QVariant> updatedFreeVarToValue; // populated in addUpdate(), cleared in compute()

    QHash<QSet<int>, QVector<int>> updatedFreeVarsToAffectedDependentVars;
            // - values are in topological order
            // - populated and used in compute(), as a cache

    // tools
    const QString getVariableName(const int varId) const {
        if (varIdHelper.isFreeVar(varId))
            return getVarName(varIdHelper.toFreeVar(varId));
        else
            return getVarName(varIdHelper.toDependentVar(varId));
    }

    void logWarning(const QString &msg) const {
        qWarning().noquote() << QString("%1: %2").arg(name, msg);
    }
};

#endif // VARIABLES_UPDATE_PROPAGATOR_H
