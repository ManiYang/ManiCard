#ifndef DIRECTED_GRAPH_H
#define DIRECTED_GRAPH_H

#include <memory>
#include <unordered_set>
#include <vector>

class DirectedGraph
{
public:
    explicit DirectedGraph();
    ~DirectedGraph();

    //!
    //! Does nothing if the vertex already exists.
    //!
    void addVertex(const int vertexId);

    //!
    //! - If \e startVertexId does not already exist, it will be added (similarly for \e endVertexId).
    //! - Does nothing if the edge already exists (i.e., parallel edges are not allowed).
    //!
    void addEdge(const int startVertexId, const int endVertexId);

    // ==== algorithms ====

    //!
    //! \return an empty vector if the graph is cyclic
    //!
    std::vector<int> topologicalOrder(const bool reverseOrder = false);

    //!
    //! Visits all the vertices of the graph.
    //! \return an empty vector if \e startingVertexId does not exist.
    //!
    std::vector<int> depthFirstTraversal(const int startingVertexId);

    //!
    //! Searches for vertices reachable from \e startingVertexId.
    //! \return an empty vector if \e startingVertexId does not exist.
    //!
    std::vector<int> breadthFirstSearch(const int startingVertexId);

private:
    class DirectedGraphImpl;
    std::unique_ptr<DirectedGraphImpl> impl;
};

//====

//!
//! A directed graph whose vertices are identified by items of an \c enum.
//! \e VertexEnum must be an \c enum type (including scoped enum) whose underlying type is \c int.
//!
template <typename VertexEnum>
class DirectedGraphWithVertexEnum
{
public:
    explicit DirectedGraphWithVertexEnum() {}

    //!
    //! Does nothing if the vertex already exists.
    //!
    void addVertex(const VertexEnum vertex) {
        graph.addVertex(static_cast<int>(vertex));
    }

    //!
    //! - If \e startVertex does not already exist, it will be added (similarly for \e endVertex).
    //! - Does nothing if the edge already exists (i.e., parallel edges are not allowed).
    //!
    void addEdge(const VertexEnum startVertex, const VertexEnum endVertex) {
        graph.addEdge(static_cast<int>(startVertex), static_cast<int>(endVertex));
    }

    // ==== algorithms ====

    //!
    //! \return an empty vector if the graph is cyclic
    //!
    std::vector<VertexEnum> topologicalOrder(const bool reverseOrder = false) {
        std::vector<int> vertices = graph.topologicalOrder(reverseOrder);

        std::vector<VertexEnum> result;
        result.reserve(vertices.size());
        for (const int v: vertices)
            result.push_back(static_cast<VertexEnum>(v));
        return result;
    }

    //!
    //! Visits all the vertices of the graph.
    //! \return an empty vector if \e startingVertex does not exist.
    //!
    std::vector<VertexEnum> depthFirstTraversal(const VertexEnum startingVertex) {
        std::vector<int> vertices = graph.depthFirstTraversal(static_cast<int>(startingVertex));

        std::vector<VertexEnum> result;
        result.reserve(vertices.size());
        for (const int v: vertices)
            result.push_back(static_cast<VertexEnum>(v));
        return result;
    }

    //!
    //! Searches for vertices reachable from \e startingVertexId.
    //! \return an empty vector if \e startingVertex does not exist.
    //!
    std::vector<VertexEnum> breadthFirstSearch(const VertexEnum startingVertex) {
        std::vector<int> vertices = graph.breadthFirstSearch(static_cast<int>(startingVertex));

        std::vector<VertexEnum> result;
        result.reserve(vertices.size());
        for (const int v: vertices)
            result.push_back(static_cast<VertexEnum>(v));
        return result;
    }

private:
    DirectedGraph graph;
};

#endif // DIRECTED_GRAPH_H
