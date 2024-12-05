#include "PartitionManager.h"
#include "ReachRatio.h"
#include <iostream>


/**
 * @brief 设置节点的分区ID。
 * @param node 节点编号。
 * @param partitionId 分区ID。
 */
PartitionManager::PartitionManager(Graph &graph):g(graph),part_g(false) 
{

}

// 建立分区图
void PartitionManager::build_partition_graph() {
    // 更新分区之间的连接
    update_partition_connections();

    // 清空之前的分区图
    part_g = Graph(false); // 假设分区图不需要存储边集


    // 填充 mapping，记录每个分区包含的节点
    for (size_t node = 0; node < g.vertices.size(); ++node) {
        int partition_id = g.get_partition_id(node);
        mapping[partition_id].insert(node);
    }

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

    
    // 添加累积后的边到分区图，但是相同的边只存一条
    for (const auto& source_pair : temp_edges) {
        int source_partition = source_pair.first;
        const auto& targets = source_pair.second;
        for (const auto& target_pair : targets) {
            int target_partition = target_pair.first;
            // 无论 edge_count 有多大，只添加一次边
            part_g.addEdge(source_partition, target_partition, true);
            //std::cout << "Added edge to partition graph: " << source_partition 
            //          << " -> " << target_partition << std::endl;
        }
    }



    // 为每个分区创建一个子图对象
    for (const auto& partition_pair : mapping) {
        int partition_id = partition_pair.first;
        partition_subgraphs[partition_id] = Graph(false); // 子图不需要存储边集
    }

    // 遍历原始图的所有节点，构建分区子图
    for (size_t u = 0; u < g.vertices.size(); ++u) {
        int u_partition = g.get_partition_id(u);

        // 获取对应的子图
        Graph& subgraph = partition_subgraphs[u_partition];

        // 确保子图中的顶点列表足够大
        if (u >= subgraph.vertices.size()) {
            subgraph.vertices.resize(u + 1);
        }

        // 复制节点分区
        subgraph.vertices[u].partition_id = u_partition;

        // 遍历出边
        for (int v : g.vertices[u].LOUT) {
            int v_partition = g.get_partition_id(v);
            if (v_partition == u_partition) {
                // 边在同一分区内，添加到子图中
                if (v >= subgraph.vertices.size()) {
                    subgraph.vertices.resize(v + 1);
                }

                // 添加边到子图
                subgraph.addEdge(u, v);
            }
        }
    }

    
    // 打印分区图信息（可选）
    std::cout << "Partition graph constructed with " << temp_edges.size() << " partitions." << std::endl;
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
                if (std::find(g.vertices[u].LOUT.begin(), g.vertices[u].LOUT.end(), v) != g.vertices[v].LOUT.end()) {
                    PartitionEdge &pe = partition_adjacency[u_partition][v_partition];
                    pe.original_edges.emplace_back(u, v);
                    pe.edge_count++;
                }else if(std::find(g.vertices[u].LIN.begin(), g.vertices[u].LIN.end(), v) != g.vertices[v].LOUT.end()){
                    PartitionEdge &pe = partition_adjacency[v_partition][u_partition];
                    pe.original_edges.emplace_back(v, u);
                    pe.edge_count++;
                }

            }
        }
    }
}

const std::unordered_map<int, PartitionEdge>& PartitionManager::getPartitionConnections(int partitionId) const {
    return partition_adjacency.at(partitionId);
}

// 获取两个分区之间的连接
const PartitionEdge& PartitionManager::get_partition_adjacency(int u, int v) const {
    auto it_outer = partition_adjacency.find(u);
    if (it_outer == partition_adjacency.end()) {
        throw std::out_of_range("Partition U not found.");
    }

    const auto& inner_map = it_outer->second;
    auto it_inner = inner_map.find(v);
    if (it_inner == inner_map.end()) {
        throw std::out_of_range("Partition V not found.");
    }

    return it_inner->second;
}