#include "ReachRatioPartitioner.h"
#include <iostream>

// 拓扑排序：对指定的节点集合进行排序
std::vector<int> ReachRatioPartitioner::topologicalSort(const Graph& graph, const std::vector<int>& nodes) {
    std::unordered_map<int, int> inDegree;
    for (int node : nodes) {
        inDegree[node] = graph.vertices[node].in_degree;
    }

    std::queue<int> q;
    for (int node : nodes) {
        if (inDegree[node] == 0) q.push(node);
    }

    std::vector<int> topoOrder;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        topoOrder.push_back(u);
        for (int v : graph.vertices[u].LOUT) {
            if (inDegree.find(v) != inDegree.end() && --inDegree[v] == 0) {
                q.push(v);
            }
        }
    }
    return topoOrder;
}

// 可达集合合并
void ReachRatioPartitioner::computeReachability(const Graph& graph,
                                                std::unordered_map<int, std::set<int>>& reachable,
                                                const std::vector<int>& topoOrder) {
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        int u = *it;
        for (int v : graph.vertices[u].LOUT) {
            reachable[u].insert(reachable[v].begin(), reachable[v].end());
        }
    }
}

// 计算第一项：分区内部可达点对数
double ReachRatioPartitioner::computeFirstTerm(const Graph& graph) {
    std::unordered_map<int, std::vector<int>> partitionNodes;
    for (size_t i = 0; i < graph.vertices.size(); ++i) {
        partitionNodes[graph.vertices[i].partition_id].push_back(i);
    }

    double firstTerm = 0.0;

    for (const auto& [partition, nodes] : partitionNodes) {
        std::unordered_map<int, std::set<int>> reachable;
        for (int node : nodes) {
            reachable[node].insert(node);
        }

        auto topoOrder = topologicalSort(graph, nodes);
        computeReachability(graph, reachable, topoOrder);

        int reachablePairs = 0;
        for (int node : nodes) {
            reachablePairs += reachable[node].size() - 1;
        }
        int V = nodes.size();
        if (V > 1) firstTerm += static_cast<double>(reachablePairs) / (V * (V - 1));
    }

    return firstTerm;
}

// 构建分区图
Graph ReachRatioPartitioner::buildPartitionGraph(const Graph& graph, const PartitionManager& partition_manager) {
    Graph partitionGraph(false);
    std::unordered_map<int, std::unordered_set<int>> edges;

    for (size_t u = 0; u < graph.vertices.size(); ++u) {
        int uPart = graph.get_partition_id(u);
        for (int v : graph.vertices[u].LOUT) {
            int vPart = graph.get_partition_id(v);
            if (uPart != vPart) {
                edges[uPart].insert(vPart);
            }
        }
    }

    for (const auto& [uPart, neighbors] : edges) {
        for (int vPart : neighbors) {
            partitionGraph.addEdge(uPart, vPart);
        }
    }

    return partitionGraph;
}

// 计算第二项：分区图上的可达点对数
double ReachRatioPartitioner::computeSecondTerm(const Graph& graph, const PartitionManager& partition_manager) {
    Graph partitionGraph = buildPartitionGraph(graph, partition_manager);

    std::unordered_map<int, std::set<int>> reachable;
    for (size_t i = 0; i < partitionGraph.vertices.size(); ++i) {
        reachable[i].insert(i);
    }

    std::vector<int> nodes;
    for (size_t i = 0; i < partitionGraph.vertices.size(); ++i) {
        nodes.push_back(i); // 将所有节点的索引加入到nodes集合中
    }

    auto topoOrder = topologicalSort(partitionGraph, nodes);
    computeReachability(partitionGraph, reachable, topoOrder);

    int reachablePairs = 0;
    for (size_t i = 0; i < partitionGraph.vertices.size(); ++i) {
        reachablePairs += reachable[i].size() - 1;
    }

    int V = partitionGraph.vertices.size();
    if (V > 1) return static_cast<double>(reachablePairs) / (V * (V - 1));
    return 0.0;
}

// 主分区函数
void ReachRatioPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    std::cout << "Starting ReachRatioPartitioner..." << std::endl;

    // 计算第一项
    double firstTerm = computeFirstTerm(graph);
    std::cout << "First Term: " << firstTerm << std::endl;

    // 计算第二项
    double secondTerm = computeSecondTerm(graph, partition_manager);
    std::cout << "Second Term: " << secondTerm << std::endl;

    // 计算最终 Q 值
    double Q = firstTerm - secondTerm;
    std::cout << "Final Q: " << Q << std::endl;

    // 可以根据 Q 值进一步优化分区逻辑（例如重新分配节点）
}
