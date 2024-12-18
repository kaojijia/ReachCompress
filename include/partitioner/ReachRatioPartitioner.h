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
    // Graph &graph;
    // PartitionManager &partition_manager;
    double computeFirstTerm(const Graph& graph);
    double computeSecondTerm(const Graph& graph, const PartitionManager& partition_manager);
    void computeReachability(const Graph &graph, const std::vector<int> &nodes, std::vector<int *> &reachableSets, std::vector<int> &reachableSizes);
    std::vector<int> topologicalSort(const Graph &graph, const std::vector<int> &nodes);
    Graph buildPartitionGraph(const Graph& graph, const PartitionManager& partition_manager);
};