#include "CompressedSearch.h"
#include "GraphPartitioner.h"
#include "LouvainPartitioner.h"  // Louvain算法的实现
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include <memory>
#include <stdexcept>

/**
 * @brief 构造函数，初始化图和分区器。
 * @param graph 要处理的图。
 */
CompressedSearch::CompressedSearch(Graph& graph) : graph_(graph) {
    // 默认使用 Louvain 分区算法，可根据需要替换
    std::string partitioner_name = "Louvain"; // 默认分区器名称
    if (partitioner_name == "Louvain") {
        partitioner_ = std::unique_ptr<LouvainPartitioner>(new LouvainPartitioner());
    } else if (partitioner_name == "GraphPartitioner") {
        throw std::invalid_argument("Unsupported partitioner name");
        //partitioner_ = std::unique_ptr<GraphPartitioner>(new GraphPartitioner());
    } else {
        throw std::invalid_argument("Unsupported partitioner name");
    }
    partitioner_ = std::unique_ptr<LouvainPartitioner>(new LouvainPartitioner());
}

/**
 * @brief 离线索引建立，执行图分区并构建辅助数据结构。
 */
void CompressedSearch::offline_industry() {
    partition_graph();  ///< 执行图分区算法
    // 构建 Bloom Filter
    bloom_filter_.build(graph_);
    // 构建 Node Embedding
    node_embedding_.build(graph_);
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

    int source_partition = partition_manager_.get_partition(source);
    int target_partition = partition_manager_.get_partition(target);

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
    partitioner_->partition(graph_, partition_manager_);
}

/**
 * @brief 在同一分区内进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_within_partition(int source, int target) {
    // 实现同分区内的可达性查询
    // ...existing code...
    return false;
}

/**
 * @brief 在不同分区间进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_across_partitions(int source, int target) {
    // 实现跨分区的可达性查询
    // ...existing code...
    return false;
}