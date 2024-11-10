#include "gtest/gtest.h"
#include "graph.h"
#include "BidirectionalBFS.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
TEST(BidirectionalBFSTest, ReachabilityTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集

    // 添加一些边
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 4);
    g.addEdge(4, 6);

    
    OutputHandler::printGraphInfo(g);

    // 创建BidirectionalBFS对象
    BidirectionalBFS bfs(g);

    // 测试可达性
    EXPECT_TRUE(bfs.reachability_query(1, 2));
    EXPECT_TRUE(bfs.reachability_query(1, 3));
    EXPECT_TRUE(bfs.reachability_query(1, 4));
    EXPECT_FALSE(bfs.reachability_query(6, 1)); // 6 -> 1 不可达（假设是有向图）
    EXPECT_TRUE(bfs.reachability_query(2, 4));  // 2 -> 4 可达
    EXPECT_FALSE(bfs.reachability_query(1, 5)); // 1 -> 5 不可达（节点5不存在）
}

TEST(BidirectionalBFSTest, NoPathTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集

    // 添加一些边
    g.addEdge(1, 2);
    g.addEdge(3, 4);

    OutputHandler::printGraphInfo(g);
    // 创建BidirectionalBFS对象
    BidirectionalBFS bfs(g);

    // 测试不可达性
    EXPECT_FALSE(bfs.reachability_query(1, 4)); // 1 -> 4 不可达
    EXPECT_FALSE(bfs.reachability_query(2, 3)); // 2 -> 3 不可达
}

TEST(BidirectionalBFSTest, SelfLoopTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集

    // 添加自环边
    g.addEdge(1, 1);
    
    OutputHandler::printGraphInfo(g);

    // 创建BidirectionalBFS对象
    BidirectionalBFS bfs(g);

    // 测试自环
    EXPECT_TRUE(bfs.reachability_query(1, 1));  // 1 -> 1 可达
}


TEST(BidirectionalBFSTest, RandomQueryPairs) {
    // 创建一个图
    Graph g(true);

    // 使用项目根目录宏构建绝对路径
    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);

    // 创建 BidirectionalBFS 对象
    BidirectionalBFS bfs(g);

    // 生成不重复的随机查询对
    int num_queries = 100;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);

    // 测试随机查询对的可达性
    for (const auto& query_pair : query_pairs) {
        int source = query_pair.first;
        int target = query_pair.second;
        bool result = bfs.reachability_query(source, target);
        std::cout << "Query from " << source << " to " << target << ": " << (result ? "Reachable" : "Not Reachable") << std::endl;
    }
}