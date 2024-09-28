#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <unordered_map>
#include <unordered_set>
#include "utilities/directed_graph.h"

template <typename Container>
bool contains(const Container &c, const typename Container::value_type &v) {
    return std::any_of(c.cbegin(), c.cend(), [v](auto item) { return item == v; });
}

TEST(DirectedGraphTests, TopologicalSort) {
    DirectedGraph graph;
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(2, 3);
    graph.addEdge(2, 5);
    graph.addEdge(4, 5);
    graph.addVertex(6);

    std::vector<int> sortedVertices = graph.topologicalOrder();
    ASSERT_TRUE(sortedVertices.size() == 7);
    ASSERT_EQ(
            std::unordered_set<int>(sortedVertices.cbegin(), sortedVertices.cend()),
            (std::unordered_set<int> {0, 1, 2, 3, 4, 5, 6})
    );

    std::unordered_map<int, int> vertexToPosition;
    for (size_t i = 0; i < sortedVertices.size(); ++i) {
        int v = sortedVertices.at(i);
        vertexToPosition[v] = i;
    }
    EXPECT_TRUE(vertexToPosition.at(0) < vertexToPosition.at(1));
    EXPECT_TRUE(vertexToPosition.at(0) < vertexToPosition.at(2));
    EXPECT_TRUE(vertexToPosition.at(2) < vertexToPosition.at(3));
    EXPECT_TRUE(vertexToPosition.at(2) < vertexToPosition.at(5));
    EXPECT_TRUE(vertexToPosition.at(4) < vertexToPosition.at(5));
}

TEST(DirectedGraphTests, TopologicalSortCyclic) {
    DirectedGraph graph;
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);

    std::vector<int> sortedVertices = graph.topologicalOrder();
    EXPECT_TRUE(sortedVertices.empty());
}

TEST(DirectedGraphTests, BreadthFirstSearch) {
    DirectedGraph graph;
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(2, 3);
    graph.addEdge(2, 5);
    graph.addEdge(4, 5);
    graph.addVertex(6);

    std::vector<int> foundVertices = graph.breadthFirstSearch(2);

    EXPECT_EQ(
            std::unordered_set<int>(foundVertices.cbegin(), foundVertices.cend()),
            (std::unordered_set<int> {2, 3, 5})
    );
}

TEST(DirectedGraphTests, VertexEnum) {
    enum class Vertex {A, B, C};
    DirectedGraphWithVertexEnum<Vertex> graph;

    graph.addVertex(Vertex::A);
    graph.addEdge(Vertex::A, Vertex::B);
    graph.addEdge(Vertex::B, Vertex::C);

    constexpr bool reverseOrder = true;
    const std::vector<Vertex> sortedVertices = graph.topologicalOrder(reverseOrder);
    EXPECT_EQ(
            sortedVertices,
            (std::vector<Vertex>{Vertex::C, Vertex::B, Vertex::A})
    );
}
