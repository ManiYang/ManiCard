#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/exception.hpp"
#include "boost/graph/topological_sort.hpp"
#include "graphs_util.h"

std::vector<int> topologicalSort(
        const std::unordered_map<int, std::unordered_set<int>> &nodeToDependencies) {
    using Graph = boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

    Graph graph;

    // add vertices
    std::unordered_set<int> allNodes;
    for (const auto &[node, dependencies]: nodeToDependencies) {
        allNodes.insert(node);
        allNodes.insert(dependencies.begin(), dependencies.end());
    }

    std::unordered_map<int, Vertex> nodeIdToVertex;
    std::unordered_map<Vertex, int> vertexToNodeId;
    for (const int node: allNodes) {
        Vertex vertex = boost::add_vertex(graph);

        nodeIdToVertex[node] = vertex;
        vertexToNodeId[vertex] = node;
    }

    // add edges
    for (const auto &[node, dependencies]: nodeToDependencies) {
        Vertex startVertex = nodeIdToVertex.at(node);
        for (const int endNode: dependencies) {
            Vertex endVertex = nodeIdToVertex.at(endNode);
            auto r = boost::add_edge(startVertex, endVertex, graph);
            Q_ASSERT(r.second);
        }
    }

    //
    std::vector<Vertex> sortedVertices;
    try {
        boost::topological_sort(graph, std::back_inserter(sortedVertices));
    } catch (boost::not_a_dag) {
        return {};
    }

    std::vector<int> result;
    result.reserve(sortedVertices.size());
    for (const Vertex &v: sortedVertices)
        result.push_back(vertexToNodeId.at(v));

    return result;
}

QVector<int> topologicalSort(const QHash<int, QSet<int>> &nodeToDependencies) {
    // convert to `std::unordered_map<int, std::unordered_set<int>>`
    std::unordered_map<int, std::unordered_set<int>> map;
    for (auto it = nodeToDependencies.constBegin(); it != nodeToDependencies.constEnd(); ++it) {
        const QSet<int> &dependencies = it.value();
        std::unordered_set<int> set;
        for (const int node: dependencies)
            set.insert(node);
        map.insert({it.key(), set});
    }

    //
    std::vector<int> result = topologicalSort(map);
    return QVector<int>(result.cbegin(), result.cend());
}
