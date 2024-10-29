#include "gtest/gtest.h"
#include "graph.h"
#include <algorithm>  // 确保包含算法库

TEST(GraphTest, AddEdgeTest) {
    Graph g(true);  // 确保存储边集
    
    
    
    g.addEdge(1, 2);
    EXPECT_EQ(std::find(g.vertices[1].LOUT.begin(), g.vertices[1].LOUT.end(), 2) != g.vertices[1].LOUT.end(), true);
    EXPECT_EQ(std::find(g.vertices[2].LIN.begin(), g.vertices[2].LIN.end(), 1) != g.vertices[2].LIN.end(), true);


    
}
