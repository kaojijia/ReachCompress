#include "gtest/gtest.h"
#include "graph.h"
#include "TreeCover.h"
#include <algorithm>  // 确保包含算法库
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"

TEST(TreeCover, test_tree_cover) {
    Graph g;
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(1, 3);
    g.addEdge(0, 4);
    g.addEdge(4, 3);
    g.addEdge(4, 5);
    TreeCover treecover(g);
    treecover.offline_industry();
    treecover.printIndex();
    EXPECT_TRUE(treecover.reachability_query(0, 5));
    EXPECT_TRUE(treecover.reachability_query(4, 5));
    EXPECT_FALSE(treecover.reachability_query(4, 3));
}