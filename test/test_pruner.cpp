#include "gtest/gtest.h"
#include "graph.h"
#include "graph_pruner.h"

TEST(GraphPrunerTest, PruneIsolatedNodes) {
    Graph g;
    g.addEdge(1, 2);
    g.addEdge(3, 4);
    g.addEdge(5, 6);  // 孤立节点

    GraphPruner pruner(g);
    pruner.pruneIsolatedNodes();

    EXPECT_EQ(g.vertices.count(5), 0);  // 孤立节点5应被移除
    EXPECT_EQ(g.vertices.count(6), 0);  // 孤立节点6应被移除
    EXPECT_EQ(g.vertices.count(1), 1);  // 连接的节点应保留
}

TEST(GraphPrunerTest, PruneByDegree) {
    Graph g;
    g.addEdge(1, 2);
    g.addEdge(3, 4);
    g.addEdge(4, 5);
    g.addEdge(6, 7);  // 度数较低的节点

    GraphPruner pruner(g);
    pruner.pruneByDegree(1, 1);  // 裁剪入度或出度小于1的节点

    EXPECT_EQ(g.vertices.count(6), 0);  // 节点6应被移除
    EXPECT_EQ(g.vertices.count(7), 0);  // 节点7应被移除
    EXPECT_EQ(g.vertices.count(1), 1);  // 连接的节点应保留
}
