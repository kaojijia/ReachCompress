#include "gtest/gtest.h"
#include "graph.h"
#include "BloomFilter.h"
#include <algorithm>  // 确保包含算法库
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"

TEST(BloomFilter, test_BLOOM) {
    Graph g(false);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);
    BloomFilter bloom(g);
    bloom.offline_industry();
    bloom.reachability_query(0,1);
    EXPECT_TRUE(bloom.reachability_query(19, 15));
    EXPECT_FALSE(bloom.reachability_query(15, 19));
    EXPECT_FALSE(bloom.reachability_query(8, 0));
}