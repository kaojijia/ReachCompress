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
    // bloom_filter_.build(g);
    // 构建 Node Embedding
    // node_embedding_.build(g);
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
    // if (!bloom_filter_.possibly_connected(source, target)) {
    //     return false;
    // }
    // if (!node_embedding_.are_nodes_related(source, target)) {
    //     return false;
    // }

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
    std::vector<int> path = part_bfs->findPath(source,target);
    if(path.empty())return false;
    auto edges = partition_manager_.get_partition_adjacency(path[0], path[1]);
    return dfs_partition_search(source, edges.original_edges, path, target);
}

/**
 * @brief 辅助方法，执行分区间的迭代式DFS搜索。
 * @param current_partition 当前分区ID。
 * @param target_partition 目标分区ID。
 * @param visited 已访问的分区集合。
 * @param path 从当前节点出发的可达路径集合
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::dfs_partition_search(int u, std::vector<std::pair<int, int>>edges, std::vector<int>path, int target) {
    auto current_partition = path[0];
    if (current_partition != g.get_partition_id(u)) {
        throw std::logic_error("Current partition from path[] does not match the partition of node u");
    }
    for(auto &edge:edges){
        //上一个前序点无法到达这条边就下一轮循环
        if(!query_within_partition(u,edge.first)) continue;
        //如果下一个分区是终点分区就使用BFS查找，如果不是指向终点分区就继续递归找
        auto next_partition = path[1];
        if(g.get_partition_id(target) == next_partition){
            return query_within_partition(edge.second, target);
        }
        else{
            std::vector<int> remain_path(path.begin() + 1, path.end());
            auto next_edges = partition_manager_.get_partition_adjacency(path[0],path[1]);
            return dfs_partition_search(edge.second,next_edges.original_edges,remain_path,target);
        }
    }

    return false;
}