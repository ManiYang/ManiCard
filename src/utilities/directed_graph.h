#ifndef DIRECTED_GRAPH_H
#define DIRECTED_GRAPH_H

#include <memory>
#include <vector>

class DirectedGraph
{
public:
    explicit DirectedGraph();

    void addVertex(const int vertexId);

    //!
    //! - If \e startVertexId does not already exist, it will be added (similarly for \e endVertexId).
    //! - Does nothing if the edge already exists (i.e., parallel edges are not allowed).
    //!
    void addEdge(const int startVertexId, const int endVertexId);

    std::vector<int> topologicalOrder();
    std::vector<int> depthFirstSearch(const int startingVertex);

private:
    class DirectedGraphImpl;
    std::unique_ptr<DirectedGraphImpl> impl;
};

#endif // DIRECTED_GRAPH_H
