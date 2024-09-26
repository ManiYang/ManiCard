#ifndef GRAPHS_UTIL_H
#define GRAPHS_UTIL_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <QHash>
#include <QSet>
#include <QVector>

//!
//! Find a topological order of the dependency graph defined by \e nodeToDependencies.
//! \param nodeToDependencies: \c nodeToDependencies[nodeId] are the node ID's that \e nodeId
//!                            depends on.
//! \return an empty vector if the graph is not a directed asyclic graph
//!
std::vector<int> topologicalSort(
        const std::unordered_map<int, std::unordered_set<int>> &nodeToDependencies);

QVector<int> topologicalSort(const QHash<int, QSet<int>> &nodeToDependencies);

#endif // GRAPHS_UTIL_H
