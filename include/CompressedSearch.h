// CompressedSearch.h
#ifndef COMPRESSED_SEARCH_H
#define COMPRESSED_SEARCH_H

#include <bitset>
#include <memory>
#include <unordered_map>
#include "pll.h"
#include "graph.h"
#include "Algorithm.h"
#include "PartitionManager.h"
#include "GraphPartitioner.h"
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include "BidirectionalBFS.h"


using namespace std;
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
    explicit CompressedSearch(Graph &graph, std::string partitioner_name = "Infomap", bool is_index = true)
        : g(graph),
          partition_manager_(graph),
          bfs(graph),
          part_bfs(nullptr),
          is_index(is_index)
    {
        set_partitioner(partitioner_name);
    }

    // 设置分区器
    void set_partitioner(std::string partitioner_name = "Infomap");

    /**
     * @brief 离线索引建立和其他辅助数据结构的初始化。
     */
    void offline_industry() override;
    
    void offline_industry(size_t num_vertices, float ratio);

    /**
     * @brief 在线查询，判断两个节点之间的可达性。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool reachability_query(int source, int target) override;


    PartitionManager& get_partition_manager() {
        return partition_manager_;
    };

    BidirectionalBFS bfs;                                 ///< 原图上的双向BFS算法。
    
    std::unordered_map<std::string, size_t> getIndexSizes() const override{
        std::unordered_map<std::string, size_t> index_sizes;

        return {};
    }

    float ratio = -1;
    size_t num_vertices = 0;
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

    /// @brief 在同一个分区内使用索引进行查询
    /// @param source 
    /// @param target 
    /// @return 
    bool query_index_within_partition(int source, int target, int partition_id); ///< 同分区查询

    /**
     * @brief 在不同分区间进行可达性查询。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool query_across_partitions(int source, int target); ///< 跨分区查询
    bool dfs_partition_search(int u, std::vector<std::pair<int, int>> edges, std::vector<int> path, int target);

    /// @brief 建立分区索引,小于num_vertices的节点做邻接矩阵,大于num_vertices的节点做索引.小于ratio的节点做PLL作为索引,大于ratio的节点不可达的邻接表作为索引
    /// @param ratio 
    /// @param num_vertices 
    void build_partition_index(float ratio, size_t num_vertices); ///< 构建分区索引

    std::unique_ptr<BidirectionalBFS> part_bfs;           ///< 分区图上的双向BFS算法。
    Graph &g;                                             ///< 处理的图。
    PartitionManager partition_manager_;                  ///< 分区管理器。
    std::unique_ptr<GraphPartitioner> partitioner_;       ///< 图分区器，支持多种分区算法。
    BloomFilter bloom_filter_;                            ///< Bloom Filter，用于快速判断节点间可能的连接。
    NodeEmbedding node_embedding_;                        ///< 节点嵌入，用于压缩邻接矩阵。
    
    unordered_map<size_t, vector<vector<size_t>>> unreachable_index_;         ///< 不可达分区索引，存储分区内的不可达点对的邻接表。
    unordered_map<size_t, unordered_map<size_t,size_t>> unreachable_mapping_;        ///< 不可达分区里面的点的映射关系，不连续点到连续的数据结构的映射关系
    unordered_map<size_t, vector<vector<bitset<1>>>> small_index_;      ///< 小分区索引,存储可达点对的邻接矩阵。
    unordered_map<size_t, unordered_map<size_t,size_t>> small_mapping_;        ///< 小分区分区里面点的映射关系，不连续点到连续的数据结构的映射关系
    std::unordered_map<size_t, PLL*> pll_index_; ///< PLL索引，用于存储每个节点的PLL索引。
    bool is_index; ///< 是否使用索引
};
#endif // COMPRESSED_SEARCH_H
