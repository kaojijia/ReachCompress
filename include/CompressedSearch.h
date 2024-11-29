// CompressedSearch.h
#ifndef COMPRESSED_SEARCH_H
#define COMPRESSED_SEARCH_H

#include "graph.h"
#include "Algorithm.h"
#include "PartitionManager.h"
#include "GraphPartitioner.h"
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include "BidirectionalBFS.h"
#include <memory>

/**
 * @class CompressedSearch
 * @brief 基于分区的压缩可达性查询算法实现。
 */
class CompressedSearch : public Algorithm
{
public:
    /**
     * @brief 构造函数，初始化图。
     * @param graph 要处理的图。
     */
    explicit CompressedSearch(Graph &graph, std::string partitioner_name = "Louvain")
        : g(graph),
          partition_manager_(graph),
          bfs(graph),
          part_bfs(nullptr)
    {
        set_partitioner(partitioner_name);
    }

    // 设置分区器
    void set_partitioner(std::string partitioner_name = "Louvain");

    /**
     * @brief 离线索引建立和其他辅助数据结构的初始化。
     */
    void offline_industry() override;

    /**
     * @brief 在线查询，判断两个节点之间的可达性。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool reachability_query(int source, int target) override;

private:
    /**
     * @brief 执行图分区算法。
     */
    void partition_graph(); ///< 图分区算法

    /**
     * @brief 在同一分区内进行可达性查询。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool query_within_partition(int source, int target); ///< 同分区查询

    /**
     * @brief 在不同分区间进行可达性查询。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool query_across_partitions(int source, int target); ///< 跨分区查询
    bool dfs_partition_search(int current_partition, int target_partition);

    BidirectionalBFS bfs;                                 ///< 原图上的双向BFS算法。
    std::unique_ptr<BidirectionalBFS> part_bfs;           ///< 分区图上的双向BFS算法。
    Graph &g;                                             ///< 处理的图。
    PartitionManager partition_manager_;                  ///< 分区管理器。
    std::unique_ptr<GraphPartitioner> partitioner_;       ///< 图分区器，支持多种分区算法。
    BloomFilter bloom_filter_;                            ///< Bloom Filter，用于快速判断节点间可能的连接。
    NodeEmbedding node_embedding_;                        ///< 节点嵌入，用于压缩邻接矩阵。
    // 其他必要的成员变量
};

#endif // COMPRESSED_SEARCH_H