#include "gtest/gtest.h"
#include "graph.h"

TEST(GraphTest, AddEdgeTest) {
    Graph g;
    g.addEdge(1, 2);
    EXPECT_EQ(g.vertices[1].LOUT.count(2), 1);
    EXPECT_EQ(g.vertices[2].LIN.count(1), 1);
}
