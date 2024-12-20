#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "partitioner/LouvainPartitioner.h"
#include "partitioner/ReachRatioPartitioner.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
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


TEST(PartitionTest, ReachRatioPartitionTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/test_square");
    inputHandler.readGraph(g);
    // OutputHandler::printGraphInfo(g);


    // 创建PartitionManager对象
    PartitionManager partition_manager(g);
    // 进行分区
    ReachRatioPartitioner partitioner;
    partitioner.partition(g, partition_manager);
    
   
    OutputHandler::printPartitionInfo(partition_manager);

}
