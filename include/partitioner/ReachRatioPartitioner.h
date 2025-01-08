#pragma once


#include "GraphPartitioner.h"
#include "graph.h"
#include "PartitionManager.h"
#include "BiBFSCSR.h"
#include "BidirectionalBFS.h"
#include "CSR.h"
#include <unordered_map>
#include <set>
#include <queue>
#include <vector>
using namespace std;

#define u32 uint32_t




class ReachRatioPartitioner : public GraphPartitioner {
public:
    ReachRatioPartitioner() : csr_(nullptr), bibfs_(nullptr), bibfs_csr_(nullptr) {};
    // 重写分区函数
    void partition(Graph& graph, PartitionManager& partition_manager) override;

    void partition_with_dfs(Graph &graph, PartitionManager &partition_manager);

    // vector<u32> dfs(u32 depth, u32 vertex, map<u32, PartitionReachable> *partition_reachable = nullptr);
    vector<u32> dfs_find_reachable_partitions(Graph& graph, u32 depth, u32 vertex, map<u32, PartitionReachable> *partition_reachable = nullptr);
    std::vector<u32> dfs_partition(Graph& graph, u32 depth, u32 vertex);
    
    
    void partition_with_maxQ(Graph &graph, PartitionManager &partition_manager);

    // PartitionManager &partition_manager;
    // Graph &graph;
    // 计算所有分区
    double computeFirstTerm(const Graph& graph);
    //计算受影响的分区
    double computeFirstTerm(const Graph &graph, int partition, PartitionManager &partition_manager);
    
    double computeSecondTerm(const Graph &graph, PartitionManager &partition_manager);

    void computeReachability(const Graph &current_graph, const std::vector<int> &nodes, std::vector<int *> &reachableSets, std::vector<int> &reachableSizes, int partition);
    std::vector<int> topologicalSort(const Graph &graph, const std::vector<int> &nodes);
    void computeReachability_BFS(const Graph &current_graph, const std::vector<int> &nodes,  std::vector<int> &reachableSizes, int partition);

    //void computeReachability_BFS_CSR(const Graph &current_graph, const std::vector<int> &nodes, std::vector<int *> &reachableSets, std::vector<int> &reachableSizes, int partition);

    // 分区不能与其他分区连边太多
    double computePartitionEdges(const Graph &graph, PartitionManager &partition_manager, int partition);
private:

    shared_ptr<BiBFSCSR> bibfs_csr_;
    shared_ptr<CSRGraph> csr_;

    shared_ptr<BidirectionalBFS> bibfs_;

    vector<u32> partitions;

};