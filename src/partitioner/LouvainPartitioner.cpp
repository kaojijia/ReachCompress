// LouvainPartitioner.cpp
#include "partitioner/LouvainPartitioner.h"
#include "PartitionManager.h"
#include "graph.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

// 计算模块度增益的函数
double LouvainPartitioner::compute_gain(int node, int target_partition, Graph& graph, PartitionManager& partition_manager, double m) {
    // 当前节点的出度
    double k_out = graph.vertices[node].LOUT.size();
    // 当前节点的入度
    double k_in = graph.vertices[node].LIN.size();

    // 节点与目标分区中节点连接的总出边数
    double sum_out = 0.0;
    for (const auto& neighbor : graph.vertices[node].LOUT) {
        if (partition_manager.get_partition_id(neighbor) == target_partition) {
            sum_out += 1.0;
        }
    }

    // 节点与目标分区中节点连接的总入边数
    double sum_in = 0.0;
    for (const auto& neighbor : graph.vertices[node].LIN) {
        if (partition_manager.get_partition_id(neighbor) == target_partition) {
            sum_in += 1.0;
        }
    }

    // 计算模块度增益（有向图）
    double delta_q = (sum_out + sum_in) / m - ((k_out + k_in) * graph.get_partition_degree(target_partition)) / (2.0 * m * m);
    return delta_q;
}

void LouvainPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    // 计算总边数 m
    double m = 0.0;
    for (const auto& vertex : graph.vertices) {
        m += vertex.LOUT.size();
    }

    // 初始化：每个节点初始在自己的社区中
    for (size_t node = 0; node < graph.vertices.size(); ++node) {
        graph.set_partition_id(node, node);
    }

    bool improvement = true;
    while (improvement) {
        improvement = false;
        for (size_t node = 0; node < graph.vertices.size(); ++node) {
            // 检查当前节点是否存在相邻节点
            if (graph.vertices[node].LOUT.empty() && graph.vertices[node].LIN.empty()) {
                continue; 
            }

            int current_partition = partition_manager.get_partition_id(node);
            std::unordered_map<int, double> partition_gain;
            // 统计邻居所属的社区及其连接数（出边）
            for (const auto& neighbor : graph.vertices[node].LOUT) {
                int neighbor_partition = partition_manager.get_partition_id(neighbor);
                if (partition_gain.find(neighbor_partition) == partition_gain.end()) {
                    partition_gain[neighbor_partition] = 0.0;
                }
                partition_gain[neighbor_partition] += 1.0;
            }
            // 统计邻居所属的社区及其连接数（入边）
            for (const auto& neighbor : graph.vertices[node].LIN) {
                int neighbor_partition = partition_manager.get_partition_id(neighbor);
                if (partition_gain.find(neighbor_partition) == partition_gain.end()) {
                    partition_gain[neighbor_partition] = 0.0;
                }
                partition_gain[neighbor_partition] += 1.0;
            }

            // 找到增益最大的社区
            int best_partition = current_partition;
            double max_gain = 0.0;
            for (const auto& [partition, count] : partition_gain) {
                double gain = compute_gain(node, partition, graph, partition_manager, m);
                if (gain > max_gain) {
                    max_gain = gain;
                    best_partition = partition;
                }
            }

            // 如果找到更好的社区且增益为正，则移动节点
            if (best_partition != current_partition && max_gain > 0.0) {
                graph.set_partition_id(node, best_partition);
                improvement = true;
            }
        }
    }

    // 建立分区图和对应的信息
    partition_manager.update_partition_connections();
    partition_manager.build_partition_graph();
}