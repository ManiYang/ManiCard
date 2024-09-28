#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/exception.hpp"
#include "boost/graph/topological_sort.hpp"
#include "directed_graph.h"

class DirectedGraph::DirectedGraphImpl
{
public:
    explicit DirectedGraphImpl() {}

    //!
    //! Does nothing if the vertex already exists.
    //!
    void addVertex(const int nodeId) {
        if (nodeIdToVertex.count(nodeId) != 0)
            return;

        Vertex vertex = boost::add_vertex(graph);
        nodeIdToVertex[nodeId] = vertex;
        vertexToNodeId[vertex] = nodeId;
    }

    //!
    //! - If \e startVertexId does not already exist, it will be added (similarly for \e endVertexId).
    //! - Does nothing if the edge already exists (i.e., parallel edges are not allowed).
    //!
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

    //!
    //! \return an empty vector if the graph is cyclic
    //!
    std::vector<int> topologicalOrder(const bool reverseOrder = false) {
        if (nodeIdToVertex.empty())
            return {};

        std::vector<Vertex> sortedVertices;
        try {
            boost::topological_sort(graph, std::back_inserter(sortedVertices));
                    // `sortedVertices` is in reversed order
        }
        catch (boost::not_a_dag) {
            return {};
        }

        std::vector<int> result;
        result.reserve(sortedVertices.size());

        if (reverseOrder) {
            for (const Vertex v: sortedVertices)
                result.push_back(vertexToNodeId.at(v));
        }
        else {
            for (auto it = sortedVertices.rbegin(); it != sortedVertices.rend(); ++it)
                result.push_back(vertexToNodeId.at(*it));
        }

        return result;
    }

    //!
    //! Visits all the vertices of the graph.
    //! \return an empty vector if \e startingVertexId does not exist.
    //!
    std::vector<int> depthFirstTraversal(const int startingNodeId) {
        if (nodeIdToVertex.count(startingNodeId) == 0)
            return {};

        Vertex startingVertex = nodeIdToVertex.at(startingNodeId);
        std::vector<Vertex> discoveredVertices;
        DepthFirstSearchVisitor visitor(discoveredVertices);
        boost::depth_first_search(graph, boost::visitor(visitor).root_vertex(startingVertex));

        std::vector<int> result;
        result.reserve(discoveredVertices.size());
        for (const Vertex v: discoveredVertices)
            result.push_back(vertexToNodeId.at(v));
        return result;
    }

    //!
    //! Searches for vertices reachable from \e startingVertexId.
    //! \return an empty vector if \e startingVertexId does not exist.
    //!
    std::vector<int> breadthFirstSearch(const int startingNodeId) {
        if (nodeIdToVertex.count(startingNodeId) == 0)
            return {};

        Vertex startingVertex = nodeIdToVertex.at(startingNodeId);
        std::vector<Vertex> discoveredVertices;
        BreadthFirstSearchVisitor visitor(discoveredVertices);
        boost::breadth_first_search(graph, startingVertex, boost::visitor(visitor));

        std::vector<int> result;
        result.reserve(discoveredVertices.size());
        for (const Vertex v: discoveredVertices)
            result.push_back(vertexToNodeId.at(v));
        return result;
    }

private:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

    struct DepthFirstSearchVisitor : public boost::dfs_visitor<>
    {
        DepthFirstSearchVisitor(std::vector<Vertex> &discoveredVertices_)
            : discoveredVertices(discoveredVertices_) {}

        void discover_vertex(Vertex v, const Graph &/*g*/) {
            discoveredVertices.push_back(v);
        }
    private:
        std::vector<Vertex> &discoveredVertices;
    };

    struct BreadthFirstSearchVisitor : public boost::bfs_visitor<>
    {
        BreadthFirstSearchVisitor(std::vector<Vertex> &discoveredVertices_)
            : discoveredVertices(discoveredVertices_) {}

        void discover_vertex(Vertex v, const Graph &/*g*/) {
            discoveredVertices.push_back(v);
        }
    private:
        std::vector<Vertex> &discoveredVertices;
    };

private:
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

DirectedGraph::~DirectedGraph() {
}

void DirectedGraph::addVertex(const int vertexId) {
    impl->addVertex(vertexId);
}

void DirectedGraph::addEdge(const int startVertexId, const int endVertexId) {
    impl->addEdge(startVertexId, endVertexId);
}

std::vector<int> DirectedGraph::topologicalOrder(const bool reverseOrder) {
    return impl->topologicalOrder(reverseOrder);
}

std::vector<int> DirectedGraph::depthFirstTraversal(const int startingVertexId) {
    return impl->depthFirstTraversal(startingVertexId);
}

std::vector<int> DirectedGraph::breadthFirstSearch(const int startingVertexId) {
    return impl->breadthFirstSearch(startingVertexId);
}
