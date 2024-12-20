#pragma once


#include "GraphPartitioner.h"
#include "graph.h"
#include "PartitionManager.h"
#include <unordered_map>
#include <set>
#include <queue>
#include <vector>
using namespace std;

class ReachRatioPartitioner : public GraphPartitioner {
public:
    // 重写分区函数
    void partition(Graph& graph, PartitionManager& partition_manager) override;

private:
    // PartitionManager &partition_manager;
    // Graph &graph;
    double computeFirstTerm(const Graph& graph);
    double computeSecondTerm(const Graph& graph, PartitionManager& partition_manager);
    void computeReachability(const Graph &current_graph, const std::vector<int> &nodes, std::vector<int *> &reachableSets, std::vector<int> &reachableSizes, int partition);
    std::vector<int> topologicalSort(const Graph &graph, const std::vector<int> &nodes);
    Graph buildPartitionGraph(const Graph& graph, PartitionManager& partition_manager);
    void computeReachability_BFS(const Graph &current_graph, const std::vector<int> &nodes, std::vector<int *> &reachableSets, std::vector<int> &reachableSizes, int partition);
};