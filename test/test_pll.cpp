#include "gtest/gtest.h"
#include "pll.h"
#include "graph.h"
#include "BidirectionalBFS.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include <string>


TEST(PLLTest, DISABLED_BIGVertexTest) {
    Graph g(true);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 4);
    g.addEdge(4, 100);
    PLL pll(g);
    pll.offline_industry();
    pll.getCurrentTimestamp();
    EXPECT_TRUE(pll.reachability_query(1, 4));
    EXPECT_TRUE(pll.reachability_query(1, 100));
    EXPECT_FALSE(pll.reachability_query(1, 5));
    EXPECT_FALSE(pll.reachability_query(1, 6));
    EXPECT_FALSE(pll.reachability_query(123, 68));
    EXPECT_FALSE(pll.reachability_query(43, 1));
    pll.getCurrentTimestamp();
}
TEST(PLLTest, DISABLED_ReachabilityTest) {
    Graph g1(true);
    Graph g2(true);

    // 使用项目根目录宏构建绝对路径
    std::string filename = "Edges/DAGs/large/tweibo-edgelist_DAG";
    // std::string gene_path = "/Edges/generate/";
    std::string result_path = "/result/";

    
    // InputHandler inputHandler1(PROJECT_ROOT_DIR + gene_path + filename);
    // inputHandler1.readGraph(g1);
    // OutputHandler outputHandler_origin(PROJECT_ROOT_DIR + result_path+ filename + "_origin");
    
    // OutputHandler::printGraphInfo(g1);
    
    InputHandler inputHandler2(PROJECT_ROOT_DIR + filename);
    inputHandler2.readGraph(g2);
    OutputHandler outputHandler_pruned(PROJECT_ROOT_DIR + result_path+ filename +"_pruned");



    // PLL pll_origin(g1);
    // pll_origin.buildPLLLabelsUnpruned();
    // outputHandler_origin.writeInOutSets(g1);
    
    PLL pll2(g2);
    pll2.buildPLLLabels();
    outputHandler_pruned.writeInOutSets(pll2);
    
    // 生成不重复的随机查询对
    int num_queries = 100;
    int max_value = g2.vertices.size();
    unsigned int seed = 42; // 可选的随机种子
        std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);

    cout<<pll2.getCurrentTimestamp()<<endl;
    for (const auto& query_pair : query_pairs) {
        int source = query_pair.first;
        int target = query_pair.second;
        bool result = pll2.query(source, target);
        std::cout << "Query from " << source << " to " << target << ": " << (result ? "Reachable" : "Not Reachable") << std::endl;
    }
    cout<<pll2.getCurrentTimestamp()<<endl;


}


TEST(PLLTest, IndexSizeTest){
    std::string filename = "/Edges/DAGs/large/tweibo-edgelist_DAG";
    Graph g(true);
    InputHandler inputHandler(PROJECT_ROOT_DIR + filename);
    inputHandler.readGraph(g);
    PLL pll(g);
    pll.buildPLLLabels();
    std::unordered_map<std::string, size_t> indexSizes = pll.getIndexSizes();
    for (const auto& [indexName, indexSize] : indexSizes) {
        std::cout << indexName << ": " << indexSize << " bytes" << std::endl;
    }

}