#include "CompressedSearch.h"
#include "GraphPartitioner.h"
#include "LouvainPartitioner.h"  // Louvain算法的实现
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <bits/unique_ptr.h>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <stack>


void CompressedSearch::set_partitioner(std::string partitioner_name)
{
    // 默认使用 Louvain 分区算法，可根据需要替换
    if (partitioner_name == "Louvain") {
        partitioner_ = std::unique_ptr<LouvainPartitioner>(new LouvainPartitioner());
    } else if (partitioner_name == "GraphPartitioner") {
        throw std::invalid_argument("Unsupported partitioner name");
        //partitioner_ = std::unique_ptr<GraphPartitioner>(new GraphPartitioner());
    } else {
        throw std::invalid_argument("Unsupported partitioner name");
    }
}

/**
 * @brief 离线索引建立，执行图分区并构建辅助数据结构。
 */
void CompressedSearch::offline_industry() {
    partition_graph();  ///< 执行图分区算法
    // 构建 Bloom Filter
    bloom_filter_.build(g);
    // 构建 Node Embedding
    node_embedding_.build(g);
    part_bfs = std::unique_ptr<BidirectionalBFS>(new BidirectionalBFS(partition_manager_.part_g));
}

/**
 * @brief 在线查询，判断两个节点之间的可达性。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::reachability_query(int source, int target) {
    // 使用 Bloom Filter 和 Node Embedding 进行快速判断
    if (!bloom_filter_.possibly_connected(source, target)) {
        return false;
    }
    if (!node_embedding_.are_nodes_related(source, target)) {
        return false;
    }

    int source_partition = g.get_partition_id(source);
    int target_partition = g.get_partition_id(target);

    if (source_partition == target_partition) {
        return query_within_partition(source, target);  ///< 同分区查询
    } else {
        return query_across_partitions(source, target); ///< 跨分区查询
    }
}

/**
 * @brief 执行图分区算法。
 */
void CompressedSearch::partition_graph() {
    partitioner_->partition(g, partition_manager_);
}

/**
 * @brief 在同一分区内进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_within_partition(int source, int target) {
    if(source == target) return true;
    if(source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0) {
        return false;
    }
    if(g.adjList[source].empty() && g.reverseAdjList[source].empty()) return false;
    if(g.adjList[target].empty() && g.reverseAdjList[target].empty()) return false;
    if(g.get_partition_id(source) != g.get_partition_id(target)) {
        std::cout<<"The vertices are not in the same partition"<<std::endl;
        return false;}
    int partition_id = g.get_partition_id(source);
    auto result = bfs.findPath(source, target, partition_id);
    if(result.size() > 0) {
        return true;
    }
    return false;
}

/**
 * @brief 在不同分区间进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_across_partitions(int source, int target) {
    auto source_partition = g.get_partition_id(source);
    auto target_partition = g.get_partition_id(target);
    auto part_path = part_bfs->findPath(source,target);
    if(part_path.size() == 0) {
        return false;
    }

    for(int i = 0; i < part_path.size() - 1; i++) {
        if(i == 0){}
        
        auto partition_id = part_path[i];
        auto next_partition_id = part_path[i + 1];
        auto result = bfs.findPath(source, target, partition_id);
        if(result.size() > 0) {
            return true;
        }
    }

    return false;
}


/**
 * @brief 辅助方法，执行分区间的迭代式DFS搜索。
 * @param current_partition 当前分区ID。
 * @param target_partition 目标分区ID。
 * @param visited 已访问的分区集合。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::dfs_partition_search(int source, int target) {
    std::stack<int> stack;
    stack.push(source);
    std::unordered_set<int> visited;

    while (!stack.empty()) {
        int current_node = stack.top();
        stack.pop();

        if (g.get_partition_id(current_node) == g.get_partition_id(target)) {
            return query_within_partition(current_node, target);
        }

        if (visited.find(partition) != visited.end()) {
            continue;
        }

        visited.insert(partition);

        // 遍历当前分区的所有相邻分区
        const auto &connections = partition_manager_.partition_adjacency[partition];
        for (const auto &pair : connections) {
            int adjacent_partition = pair.first;
            const PartitionEdge &edge = pair.second;

            // 遍历连接边，检查分区内的可达性
            for (const auto &edge_pair : edge.original_edges) {
                int from_node = edge_pair.first;
                int to_node = edge_pair.second;

                // 检查从当前分区的某个节点是否可达
                if (query_within_partition(from_node, to_node)) {
                    if (visited.find(adjacent_partition) == visited.end()) {
                        stack.push(adjacent_partition);
                    }
                }
            }
        }
    }

    return false;
}