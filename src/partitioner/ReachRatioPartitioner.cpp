#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <set>
#include <cmath>
#include <random>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "graph.h"
#include "partitioner/ReachRatioPartitioner.h"

// 获取当前时间戳
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
    return ss.str();
}



// 独立的可达性计算模块
void ReachRatioPartitioner::computeReachability(const Graph& graph,
                         const std::vector<int>& nodes,
                         std::vector<int*>& reachableSets,
                         std::vector<int>& reachableSizes) {
    // 初始化每个节点的可达集合
    for (size_t i = 0; i < nodes.size(); ++i) {
        int node = nodes[i];
        reachableSets[i] = new int[1]; // 动态数组初始化
        reachableSets[i][0] = node;   // 初始集合仅包含自身
        reachableSizes[i] = 1;
    }

    // 拓扑排序：仅对当前分区的节点排序
    auto topoOrder = topologicalSort(graph, nodes);

    // 按拓扑排序逆序合并可达集合
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        int u = *it; // 当前节点

        // 检查索引合法性
        if (u < 0 || u >= nodes.size()) {
            continue;
        }

        std::set<int> mergedSet;

        // 包含自身
        mergedSet.insert(u);

        // 遍历当前节点的后继节点
        for (int v : graph.vertices[u].LOUT) {
            // 限制只处理当前分区的节点
            if (graph.vertices[u].partition_id == graph.vertices[v].partition_id) {
                // 合并后继节点的可达集合
                for (int i = 0; i < reachableSizes[v]; ++i) {
                    mergedSet.insert(reachableSets[v][i]);
                }
            }
        }

        // 将合并后的集合存储到动态数组中
        if (reachableSets[u] != nullptr) {
            delete[] reachableSets[u]; // 释放旧内存
            reachableSets[u] = nullptr; // 防止悬挂指针
        }

        reachableSizes[u] = mergedSet.size(); // 更新大小
        reachableSets[u] = new int[mergedSet.size()]; // 分配新内存

        size_t idx = 0;
        for (int val : mergedSet) {
            reachableSets[u][idx++] = val;
        }

        // 输出调试信息
        std::cout << "Node " << u << " reachable set size: " << reachableSizes[u] << std::endl;
    }
}



// 拓扑排序：对指定的节点集合进行排序
std::vector<int> ReachRatioPartitioner::topologicalSort(const Graph& graph, const std::vector<int>& nodes) {
    // 初始化入度
    std::unordered_map<int, int> inDegree;
    for (int node : nodes) {
        inDegree[node] = 0; // 初始化入度为 0
    }
    for (int node : nodes) {
        for (int neighbor : graph.vertices[node].LOUT) {
            if (inDegree.find(neighbor) != inDegree.end()) {
                inDegree[neighbor]++;
            }
        }
    }

    // 打印初始入度信息
    std::cout << "Initial in-degree values:" << std::endl;
    for (const auto& [node, degree] : inDegree) {
        std::cout << "Node " << node << ": " << degree << std::endl;
    }

    // 初始化队列和拓扑顺序
    std::queue<int> q;
    for (int node : nodes) {
        if (inDegree[node] == 0) {
            q.push(node);
        }
    }

    std::vector<int> topoOrder;

    // 拓扑排序
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        topoOrder.push_back(u);

        for (int v : graph.vertices[u].LOUT) {
            if (inDegree.find(v) != inDegree.end()) {
                --inDegree[v];
                std::cout << "Reduced in-degree of node " << v << " to " << inDegree[v] << std::endl;
                if (inDegree[v] == 0) {
                    q.push(v);
                }
            }
        }
    }

    // 检查是否存在环
    if (topoOrder.size() != nodes.size()) {
        std::cerr << "Error: Graph contains a cycle or incomplete input. Topological sorting failed." << std::endl;
        exit(EXIT_FAILURE);
    }

    return topoOrder;
}




Graph ReachRatioPartitioner::buildPartitionGraph(const Graph& graph, const PartitionManager& partition_manager) {
    Graph partitionGraph(false); // 分区图初始化（无向图）
    std::unordered_map<int, std::unordered_set<int>> edges;

    // 遍历所有节点，构建分区之间的边
    for (size_t u = 0; u < graph.vertices.size(); ++u) {
        int uPart = graph.get_partition_id(u); // 获取节点 u 的分区号
        for (int v : graph.vertices[u].LOUT) {
            int vPart = graph.get_partition_id(v); // 获取后继节点 v 的分区号
            if (uPart != vPart && uPart != -1 && vPart != -1) { // 忽略无效分区或同分区的边
                edges[uPart].insert(vPart);
            }
        }
    }

    // 将分区之间的边加入分区图
    for (const auto& [uPart, neighbors] : edges) {
        for (int vPart : neighbors) {
            partitionGraph.addEdge(uPart, vPart);
        }
    }

    for(auto& node : partitionGraph.vertices){
        node.partition_id = 1;
    }

    return partitionGraph;
}

double ReachRatioPartitioner::computeFirstTerm(const Graph& graph) {
    std::unordered_map<int, std::vector<int>> partitionNodes;

    // 收集每个分区的节点
    for (size_t i = 0; i < graph.vertices.size(); ++i) {
        if (graph.vertices[i].partition_id != -1) { // 忽略未分配分区的节点
            partitionNodes[graph.vertices[i].partition_id].push_back(i);
        }
    }

    double firstTerm = 0.0;

    // 遍历每个分区
    for (const auto& [partition, nodes] : partitionNodes) {
        std::vector<int*> reachableSets(nodes.size(), nullptr);
        std::vector<int> reachableSizes(nodes.size(), 0);

        // 检查当前分区是否为空
        if (nodes.empty()) {
            std::cerr << "Warning: Partition " << partition << " is empty." << std::endl;
            continue;
        }

        // 调用可达性计算模块
        computeReachability(graph, nodes, reachableSets, reachableSizes);

        // 统计分区内部的可达点对数
        int reachablePairs = 0;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (reachableSizes[i] > 0) {
                reachablePairs += reachableSizes[i] - 1; // 排除自身
            }
        }

        // 计算当前分区的贡献
        int V = graph.get_num_vertices();
        if (V > 1) {
            firstTerm += static_cast<double>(reachablePairs) / (V * (V - 1));
        }

        // 释放内存，确保合法性检查
        for (size_t i = 0; i < reachableSets.size(); ++i) {
            if (reachableSets[i] != nullptr) {
                delete[] reachableSets[i];
                reachableSets[i] = nullptr; // 避免悬挂指针
            }
        }
    }

    return firstTerm;
}

double ReachRatioPartitioner::computeSecondTerm(const Graph& graph, const PartitionManager& partition_manager) {
    // 构建分区图
    Graph partitionGraph = buildPartitionGraph(graph, partition_manager);

    std::vector<int> nodes;
    for (size_t i = 0; i < partitionGraph.vertices.size(); ++i) {
        nodes.push_back(i); // 将分区图的节点加入集合
    }

    std::vector<int*> reachableSets(nodes.size(), nullptr);
    std::vector<int> reachableSizes(nodes.size(), 0);

    // 调用可达性计算模块
    computeReachability(partitionGraph, nodes, reachableSets, reachableSizes);

    // 统计分区图上的可达点对数
    int reachablePairs = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        reachablePairs += reachableSizes[i]-1;
    }

    // 释放内存
    for (size_t i = 0; i < reachableSets.size(); ++i) {
        delete[] reachableSets[i];
    }

    int V = graph.get_num_vertices();
    if (V > 1) return static_cast<double>(reachablePairs) / (V * (V - 1));
    return 0.0;
}


void ReachRatioPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    std::cout << "Starting ReachRatioPartitioner..." << std::endl;

    std::unordered_map<int, int> partitionSizes; // 分区号 -> 分区中节点数

    // 初始化分区
    for (size_t i = 0; i < graph.vertices.size(); ++i) {
        if (graph.vertices[i].out_degree > 0 || graph.vertices[i].in_degree > 0) {
            graph.set_partition_id(i, i); // 初始分区号为节点自身编号
            partitionSizes[i] = 1;        // 每个节点独占一个分区
        } else {
            graph.set_partition_id(i, -1); // 无效节点分区号为 -1
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<double> recentQs;
    const size_t maxHistorySize = 1000;
    const double fluctuationThreshold = 0.001;
    const int maxIterations = 1000;

    double currentQ = computeFirstTerm(graph) - computeSecondTerm(graph, partition_manager);
    recentQs.push_back(currentQ);

    int iterationCount = 0;
    bool improvement = true;

    while (improvement && iterationCount < maxIterations) {
        iterationCount++;
        std::cout << "Iteration: " << iterationCount << ", Current Q: " << currentQ << std::endl;
        improvement = false;

        std::vector<int> movableNodes;
        for (size_t i = 0; i < graph.vertices.size(); ++i) {
            int partition = graph.vertices[i].partition_id;
            if (partition != -1 && partitionSizes[partition] == 1) {
                movableNodes.push_back(i);// 将分区中只有一个节点的节点加入
            }
        }

        std::shuffle(movableNodes.begin(), movableNodes.end(), gen);

        for (int node : movableNodes) {
            int originalPartition = graph.get_partition_id(node);
            std::unordered_set<int> neighborPartitions;
            for (int neighbor : graph.vertices[node].LOUT) {
                int neighborPartition = graph.get_partition_id(neighbor);
                if (neighborPartition != -1 && neighborPartition != originalPartition) {
                    neighborPartitions.insert(neighborPartition);
                }
            }

            for (int targetPartition : neighborPartitions) {
                graph.set_partition_id(node, targetPartition);
                partitionSizes[originalPartition]--;
                partitionSizes[targetPartition]++;

                double newQ = computeFirstTerm(graph) - computeSecondTerm(graph, partition_manager);

                if (newQ > currentQ) {
                    currentQ = newQ;
                    recentQs.push_back(newQ);
                    if (recentQs.size() > maxHistorySize) {
                        recentQs.erase(recentQs.begin());
                    }
                    improvement = true;
                    break;
                } else {
                    graph.set_partition_id(node, originalPartition);
                    partitionSizes[originalPartition]++;
                    partitionSizes[targetPartition]--;
                }
            }

            if (improvement) break;
        }

        if (recentQs.size() == maxHistorySize) {
            double fluctuation = 0.0;
            for (size_t i = 1; i < recentQs.size(); ++i) {
                fluctuation += std::abs(recentQs[i] - recentQs[i - 1]);
            }
            fluctuation /= (recentQs.size() - 1);

            if (fluctuation < fluctuationThreshold) {
                std::cout << "Partitioning converged after " << iterationCount << " iterations. Final Q: " << currentQ << std::endl;
                break;
            }
        }
    }
    partition_manager.build_partition_graph();
}
