#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "partitioner/LouvainPartitioner.h"
#include "partitioner/RandomPartitioner.h"
#include "partitioner/ReachRatioPartitioner.h"
#include "partitioner/TraversePartitioner.h"
#include "pll.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include "CompressedSearch.h"
TEST(PartitionTest, DISABLED_LouvainTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);


    // 创建PartitionManager对象
    PartitionManager partition_manager(g);
    // 进行分区
    LouvainPartitioner partitioner;
    partitioner.partition(g, partition_manager);
    
    auto edges = partition_manager.get_partition_adjacency(3,16);
   
    OutputHandler::printPartitionInfo(partition_manager);

}


TEST(PartitionTest, PartitionerTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/DAGs/medium/cit-DBLP_DAG");
    inputHandler.readGraph(g);
    // OutputHandler::printGraphInfo(g);


    // 创建PartitionManager对象
    PartitionManager partition_manager(g);
    // 进行分区
    // ReachRatioPartitioner partitioner;
    // RandomPartitioner partitioner;
    // partitioner.partition(g, partition_manager);
    CompressedSearch comps(g, "Random");
    comps.offline_industry(200, 0.6);
    // PLL pll(g);
    // pll.offline_industry();
    // auto pll_index = pll.getIndexSizes();
    // cout<<endl;
    // cout<<"PLL index size: "<<endl;
    // for(auto [key, value] : pll_index){
    //     cout<<key<<"  "<<value<<endl;
    // }
    //     cout<<endl;
    cout<<"Compressed index size: "<<endl;
    // OutputHandler::printPartitionInfo(partition_manager);
    auto index = comps.getIndexSizes();
    for(auto [key, value] : index){
        cout<<key<<"  "<<value<<endl;
    }
    

    // auto r1 = comps.reachability_query(1, 18);
    // auto r2 = pll.reachability_query(1, 18);
    // EXPECT_TRUE(r1 == r2);


}
