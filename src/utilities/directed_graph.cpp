#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/exception.hpp"
#include "boost/graph/topological_sort.hpp"
#include "directed_graph.h"

class DirectedGraph::DirectedGraphImpl
{
public:
    explicit DirectedGraphImpl() {}

    void addVertex(const int nodeId) {
        Vertex vertex = boost::add_vertex(graph);
        nodeIdToVertex[nodeId] = vertex;
        vertexToNodeId[vertex] = nodeId;
    }

    void addEdge(const int startNodeId, const int endNodeId) {
        if (nodeIdToVertex.count(startNodeId) == 0)
            addVertex(startNodeId);
        if (nodeIdToVertex.count(endNodeId) == 0)
            addVertex(endNodeId);

        Vertex startVertex = nodeIdToVertex.at(startNodeId);
        Vertex endVertex = nodeIdToVertex.at(endNodeId);
        if (!hasEdge(startVertex, endVertex))
            boost::add_edge(startVertex, endVertex, graph);
    }

    std::vector<int> topologicalOrder();
    std::vector<int> depthFirstSearch(const int startingVertex);

private:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

    Graph graph;

    std::unordered_map<int, Vertex> nodeIdToVertex;
    std::unordered_map<Vertex, int> vertexToNodeId;

    bool hasEdge(Vertex startVertex, Vertex endVertex) const {
        for (auto [it, itEnd] = boost::adjacent_vertices(startVertex, graph); it != itEnd; ++it) {
            if (*it == endVertex)
                return true;
        }
        return false;
    }
};

//====

DirectedGraph::DirectedGraph()
    : impl(std::make_unique<DirectedGraphImpl>()) {
}

void DirectedGraph::addVertex(const int vertexId) {
    impl->addVertex(vertexId);
}

void DirectedGraph::addEdge(const int startVertexId, const int endVertexId) {
    impl->addEdge(startVertexId, endVertexId);
}

std::vector<int> DirectedGraph::topologicalOrder() {
    return impl->topologicalOrder();
}

std::vector<int> DirectedGraph::depthFirstSearch(const int startingVertex) {
    return impl->depthFirstSearch(startingVertex);
}
