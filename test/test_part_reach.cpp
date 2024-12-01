#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "LouvainPartitioner.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include "CompressedSearch.h"

TEST(ReachabilityTest, BasicTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);
    // // 创建PartitionManager对象
    // PartitionManager partition_manager(g);
    // // 进行分区
    // LouvainPartitioner partitioner;
    // partitioner.partition(g, partition_manager);
    
    // auto edges = partition_manager.get_partition_adjacency(3,16);
   
    // OutputHandler::printPartitionInfo(partition_manager);

    CompressedSearch comps(g);
    comps.offline_industry();
    EXPECT_TRUE(comps.reachability_query(3,16));
    EXPECT_TRUE(comps.reachability_query(3,18));
    EXPECT_TRUE(comps.reachability_query(3,11));
    
    // EXPECT_FALSE(comps.reachability_query(3,0));

    // //这几个出问题了
    comps.reachability_query(3,1);
    comps.reachability_query(3,8);
    comps.reachability_query(3,9);
    comps.reachability_query(3,19);
    comps.reachability_query(3,12);
    comps.reachability_query(3,7);
    comps.reachability_query(0,1);
    comps.reachability_query(0,17);
    comps.reachability_query(0,17);

    int num_queries = 100;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);


    for (const auto& query_pair : query_pairs) {
        int source = query_pair.first;
        int target = query_pair.second;

        std::cout << "query from "<< source << " to " <<target<<std::endl;
        bool bfs_result = comps.bfs.reachability_query(source, target);
        bool part_result = comps.reachability_query(source, target);
        EXPECT_EQ(bfs_result,part_result);
        std::cout << "BiBFS :"<< (bfs_result ? " Reachable" : " Not Reachable") << std::endl;
        std::cout << "PartSearch:" << (part_result ? " Reachable" : " Not Reachable") << std::endl;
        std::cout << "***** Does Result Match from " << source << " query to " << target << ":  " << ((bfs_result==part_result)?"Match":"Not Match") <<" *****"<< std::endl;
        std::cout << std::endl;
        // if (!path.empty()) {
        //     std::cout << "Path found: ";
        //     for (int node : path) {
        //         std::cout << node << " ";
        //     }
        //     std::cout << std::endl;
        // } else {
        //     std::cout << "No path exists" << std::endl;
        // }

    }



}
