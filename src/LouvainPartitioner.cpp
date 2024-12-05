// LouvainPartitioner.cpp
#include "LouvainPartitioner.h"
#include "PartitionManager.h"
#include "graph.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>
void LouvainPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    // 计算总边数 m
    double m = 0.0;
    for (const auto& vertex : graph.vertices) {
        m += vertex.LOUT.size();
    }

    // 初始化：每个节点初始在自己的社区中，并初始化社区统计信息
    for (size_t node = 0; node < graph.vertices.size(); ++node) {
        partition_manager.set_partition(node, node);
        //partition_manager.update_community_stats(node, graph.vertices[node].LIN.size(), graph.vertices[node].LOUT.size(), 0.0);
    }

    bool improvement = true;
    while (improvement) {
        improvement = false;
        for (size_t node = 0; node < graph.vertices.size(); ++node) {
            if (graph.vertices[node].LOUT.empty() && graph.vertices[node].LIN.empty()) {
                continue;
            }

            int current_partition = partition_manager.get_partition(node);
            std::unordered_map<int, double> partition_gain;

            // 统计节点与各邻居社区的连接
            for (const auto& neighbor : graph.vertices[node].LOUT) {
                int neighbor_partition = partition_manager.get_partition(neighbor);
                partition_gain[neighbor_partition] += 1.0;
            }

            for (const auto& neighbor : graph.vertices[node].LIN) {
                int neighbor_partition = partition_manager.get_partition(neighbor);
                partition_gain[neighbor_partition] += 1.0;
            }

            // 找到增益最大的社区
            int best_partition = current_partition;
            double max_gain = 0.0;
            for (const auto& [partition, _] : partition_gain) {
                double gain = compute_gain(node, partition, graph, partition_manager, m);
                if (gain > max_gain) {
                    max_gain = gain;
                    best_partition = partition;
                }
            }

            // 如果找到更好的社区且增益为正，则移动节点
            if (best_partition != current_partition && max_gain > 0.0) {
                // 更新社区统计信息
                // partition_manager.update_community_stats(current_partition,
                //                                           -graph.vertices[node].LIN.size(),
                //                                           -graph.vertices[node].LOUT.size(),
                //                                           0.0);
                // partition_manager.update_community_stats(best_partition,
                //                                           graph.vertices[node].LIN.size(),
                //                                           graph.vertices[node].LOUT.size(),
                //                                           0.0);

                // 更新节点的社区
                partition_manager.set_partition(node, best_partition);
                improvement = true;
            }
        }
    }

    // 建立分区图和对应的信息
    partition_manager.build_partition_graph();
}
double LouvainPartitioner::compute_gain(
    int node,
    int target_partition,
    Graph& graph,
    PartitionManager& partition_manager,
    double m
) {
    // 当前节点的出度和入度
    double k_out = graph.vertices[node].LOUT.size();
    double k_in = graph.vertices[node].LIN.size();

    // 节点与目标分区的入度和出度
    double sum_out = 0.0, sum_in = 0.0;

    for (const auto& neighbor : graph.vertices[node].LOUT) {
        if (partition_manager.get_partition(neighbor) == target_partition) {
            sum_out += 1.0;
        }
    }

    for (const auto& neighbor : graph.vertices[node].LIN) {
        if (partition_manager.get_partition(neighbor) == target_partition) {
            sum_in += 1.0;
        }
    }

    // 从缓存中获取目标社区的统计信息
    //double target_in_degree = partition_manager.get_community_in_degree(target_partition);
    //double target_out_degree = partition_manager.get_community_out_degree(target_partition);

    // 计算模块度增益（有向图）
    //double delta_q = (sum_out + sum_in) / m -
  //                   ((k_out + k_in) * (target_in_degree + target_out_degree)) / (2.0 * m * m);
    return 1.0f;
    //return delta_q;
}
