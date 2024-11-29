#include "PartitionManager.h"
#include <iostream>

/**
 * @brief 设置节点的分区ID。
 * @param node 节点编号。
 * @param partitionId 分区ID。
 */
PartitionManager::PartitionManager(Graph &graph):g(graph),part_g(false) 
{
    int a;
}

// 建立分区图
void PartitionManager::build_partition_graph() {
    // 更新分区之间的连接
    update_partition_connections();

    // 清空之前的分区图
    part_g = Graph(false); // 假设分区图不需要存储边集

    // 使用一个临时的映射来累积边的权重
    std::unordered_map<int, std::unordered_map<int, int>> temp_edges;

    // 遍历分区之间的连接，累积边的数量
    for (const auto& source_pair : partition_adjacency) {
        int source_partition = source_pair.first;
        const auto& targets = source_pair.second;
        for (const auto& target_pair : targets) {
            int target_partition = target_pair.first;
            const auto& edge_info = target_pair.second;
            temp_edges[source_partition][target_partition] += edge_info.edge_count;
        }
    }

    // 调试输出 temp_edges
    std::cout << "Temp Edges:" << std::endl;
    for (const auto& source_pair : temp_edges) {
        int source_partition = source_pair.first;
        const auto& targets = source_pair.second;
        for (const auto& target_pair : targets) {
            int target_partition = target_pair.first;
            int edge_count = target_pair.second;
            std::cout << "Temp Edge: Partition " << source_partition << " -> Partition " 
                      << target_partition << " with count " << edge_count << std::endl;
        }
    }

    // 添加累积后的边到分区图，但是相同的边只存一条
    for (const auto& source_pair : temp_edges) {
        int source_partition = source_pair.first;
        const auto& targets = source_pair.second;
        for (const auto& target_pair : targets) {
            int target_partition = target_pair.first;
            // 无论 edge_count 有多大，只添加一次边
            part_g.addEdge(source_partition, target_partition, true);
            std::cout << "Added edge to partition graph: " << source_partition 
                      << " -> " << target_partition << std::endl;
        }
    }

    // 打印分区图信息（可选）
    std::cout << "Partition graph constructed with " << temp_edges.size() << " partitions." << std::endl;
}
void PartitionManager::set_partition(int node, int partition_id)
{
    g.set_partition_id(node, partition_id);
}

void PartitionManager::update_partition_connections() {
    for (size_t u = 0; u < g.adjList.size(); ++u) {
        if (g.adjList[u].empty()) {
            continue; // Skip nodes with no edges
        }
        int u_partition = g.get_partition_id(u);
        for (const auto &v : g.adjList[u]) {
            int v_partition = g.get_partition_id(v);
            if (u_partition != v_partition) {
                PartitionEdge &pe = partition_adjacency[u_partition][v_partition];
                pe.original_edges.emplace_back(u, v);
                pe.edge_count++;
            }
        }
    }
}

const std::unordered_map<int, PartitionEdge>& PartitionManager::getPartitionConnections(int partitionId) const {
    return partition_adjacency.at(partitionId);
}