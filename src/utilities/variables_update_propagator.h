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

/**
 * todo:
 * - add unit test
 * - not every var is cached
 * - incremental update
 */


//==== example ====
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

//inline uint qHash(const QSet<Var> &varSet, uint seed) {
//    QSet<int> intSet;
//    for (Var v: varSet)
//        intSet << static_cast<int>(v);
//    return qHash(intSet, seed);
//}

struct BType
{
    BType() {}
    BType(const double &v_) : v(v_) {}
    double value() const { return v; }
private:
    double v {std::nan("")};
};

Q_DECLARE_METATYPE(BType);
//==== END example ====



//========

template <typename VarEnum>
class VariablesUpdatePropagator;

template <typename VarEnum>
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
    friend class VariablesUpdatePropagator<VarEnum>;

    QHash<VarEnum, QVariant> *const valuesPtr;

    bool isPreparingVariables {true};

    QSet<VarEnum> inputVars; // used only when `isPreparingVariables` is true
    QSet<VarEnum> outputVars;  // used only when `isPreparingVariables` is true

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
            Q_ASSERT(false);
            return *this;
        }

        freeVariables << var;
        values.insert(var, QVariant::fromValue(initValue));
        return *this;
    }

    using VarComputeFunc = std::function<void (VariablesAccess<VarEnum> &varsAccess)>;

    //!
    //! Registers a dependent variable and sets the function that computes it.
    //! Example of `func`:
    //!   void computeX(VariablesAccess<Var> &varsAccess)
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
        if (isInitialized) {
            qWarning().noquote() << "VariablesUpdatePropagator already initialized";
            Q_ASSERT(false);
            return;
        }

        initializeOk = true; // can be set to false in following steps

        // create graph
        for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it) {
            VarEnum dependentVar = it.key();
            const QSet<VarEnum> &varSet = it.value();
            for (const VarEnum var: varSet)
                graph.addEdge(var, dependentVar);
        }

        // find a topological order
        const QSet<VarEnum> allVars = keySet(dependencies) | freeVariables;
        {
            const std::vector<VarEnum> sortedVars = graph.topologicalOrder(); // empty if failed
            if (!allVars.isEmpty() && sortedVars.empty()) {
                qWarning().noquote() << "the dependency graph is cyclic";
                Q_ASSERT(false);
                initializeOk = false;
            }

            dependentVarsInTopologicalOrder.reserve(sortedVars.size());
            for (const VarEnum var: sortedVars) {
                if (dependencies.contains(var))
                    dependentVarsInTopologicalOrder << var;
            }
        }

        // finish initialization
        variablesAccess.isPreparingVariables = false;
        isInitialized = true;

        // compute initial values of dependent variables
        for (const VarEnum var: dependentVarsInTopologicalOrder)
            functions[var](variablesAccess);

        if (initializeOk)
            Q_ASSERT(keySet(values) == allVars); // check that every variable has been initialized
    }

    // ==== get ====
    bool hasVar(const VarEnum var) const {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        return values.contains(var);
    }

    template <typename ValueType>
    ValueType getValue(const VarEnum var) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (!initializeOk) {
            qWarning().noquote() << "initialization was not successful";
            return ValueType();
        }

        Q_ASSERT(values.contains(var));
        return values[var].template value<ValueType>();
    }

    template <typename ValueType>
    QVariant getValueAsVariant(const VarEnum var) {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (!initializeOk) {
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
        if (!initializeOk) {
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
    //! \return updated variables
    //!
    QSet<VarEnum> compute() {
        Q_ASSERT(isInitialized); // must call initialize() beforehand
        if (!initializeOk) {
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

        //
        const QVector<VarEnum> varsToCompute
                = updatedFreeVarsToAffectedDependentVars.value(updatedFreeVarsSet);
        for (const auto var: varsToCompute) {
            if (!functions.contains(var)) {
                qWarning().noquote() << QString("`functions[%1]` not found").arg(getVarName(var));
                continue;
            }
            functions.value(var)(variablesAccess);
        }

        //
        updatedFreeVarToValue.clear();
        return QSet<VarEnum>(varsToCompute.constBegin(), varsToCompute.constEnd());
    }

private:
    bool isInitialized {false}; // has initialize() been called?
    bool initializeOk {true}; // is initialization successful?

    QSet<VarEnum> freeVariables; // populated during initialization

    QHash<VarEnum, VarComputeFunc> functions;
    QHash<VarEnum, QSet<VarEnum>> dependencies;
            // - keys are the dependent variables
            // - populated during initialization

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
