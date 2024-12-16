#include "gtest/gtest.h"
#include "graph.h"
#include "TreeCover.h"
#include <algorithm>  // 确保包含算法库
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"

TEST(TreeCover, test_tree_cover) {
    Graph g(false);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);
    TreeCover treecover(g);
    treecover.offline_industry();
    treecover.printIndex();
    EXPECT_TRUE(treecover.reachability_query(0, 5));
    EXPECT_TRUE(treecover.reachability_query(4, 5));
    EXPECT_FALSE(treecover.reachability_query(4, 3));
}