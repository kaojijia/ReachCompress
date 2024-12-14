// CompressedSearch.h
#ifndef COMPRESSED_SEARCH_H
#define COMPRESSED_SEARCH_H
#include <sstream>
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
#include "BiBFSCSR.h"

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
    explicit CompressedSearch(Graph &graph, std::string partitioner_name = "Import", bool is_index = true)
        : g(graph),
          partition_manager_(graph),
          bfs(graph),
          part_bfs(nullptr),
          is_index(is_index)
    {
        set_partitioner(partitioner_name);
    }

    // 设置分区器
    void set_partitioner(std::string partitioner_name);

    /**
     * @brief 离线索引建立和其他辅助数据结构的初始化。
     */
    void offline_industry() override;

    void offline_industry(size_t num_vertices, float ratio);

    void construct_filter();

    /**
     * @brief 在线查询，判断两个节点之间的可达性。
     * @param source 源节点。
     * @param target 目标节点。
     * @return 如果可达返回 true，否则返回 false。
     */
    bool reachability_query(int source, int target) override;
    std::vector<std::string> get_index_info();

    PartitionManager &get_partition_manager()
    {
        return partition_manager_;
    };

    BidirectionalBFS bfs; ///< 原图上的双向BFS算法。

    std::unordered_map<std::string, size_t> getIndexSizes() const override
    {
        std::unordered_map<std::string, size_t> index_sizes;
        index_sizes["PLL"] = 0;
        index_sizes["Small Index"] = 0;
        index_sizes["Unreachable Index"] = 0;
        index_sizes["G'CSR"] = 0;
        index_sizes["Partition Mapping"] = 0;
        index_sizes["Partition Connection"] = 0;
        index_sizes["Equivalence Mapping"] = 0;

        // 计算 PLL 索引的大小
        for (const auto &pll : pll_index_)
        {
            auto i = pll.second->getIndexSizes();
            index_sizes["PLL"] += i["IN"] + i["OUT"];
        }

        // 计算小分区索引的大小
        for (const auto &small_index : small_index_)
        {
            for (const auto &row : small_index.second)
            {
                index_sizes["Small Index"] += row.size() * sizeof(std::bitset<1>);
            }
        }

        // 计算不可达索引的大小
        for (const auto &unreachable_index : unreachable_index_)
        {
            for (const auto &row : unreachable_index.second)
            {
                index_sizes["Unreachable Index"] += row.size() * sizeof(size_t);
            }
        }

        // 计算分区图的大小
        index_sizes["G'CSR"] += part_bfs->getIndexSizes()["G'CSR"];

        // 计算分区信息的大小
        index_sizes["Partition Mapping"] += partition_manager_.mapping.size() * sizeof(std::unordered_set<int>);

        // 计算分区间的联系的大小
        index_sizes["Partition Connection"] += partition_manager_.partition_adjacency.size() * sizeof(std::unordered_map<int, PartitionEdge>);

        // 计算等价类大小
        index_sizes["Equivalence Mapping"] += partition_manager_.get_equivalence_mapping_size();

        return index_sizes;
    }
    void read_equivalance_info(string filename)
    {
        this->partition_manager_.read_equivalance_info(filename);
        this->partition_manager_.print_equivalence_mapping();
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

    /**
     * @brief 跨分区查询，但是可以对每个路径进行遍历查询，上面那个如果有一个路径不行了那就直接输出false
     *
     * @param source
     * @param target
     * @return true
     * @return false
     */
    bool query_across_partitions_with_all_paths(int source, int target);

    bool dfs_paths_search(int current_partition, int target_partition, std::vector<int> &path, std::vector<std::vector<int>> &all_paths, std::unordered_set<int> &visited, int source, int target);
    bool dfs_partition_search(int u, std::vector<std::pair<int, int>> edges, std::vector<int> path, int target);

    /// @brief 建立分区索引,小于num_vertices的节点做邻接矩阵,大于num_vertices的节点做索引.小于ratio的节点做PLL作为索引,大于ratio的节点不可达的邻接表作为索引
    /// @param ratio
    /// @param num_vertices
    void build_partition_index(float ratio, size_t num_vertices); ///< 构建分区索引

    // std::unique_ptr<BidirectionalBFS> part_bfs;           ///< 分区图上的双向BFS类。
    std::unique_ptr<BiBFSCSR> part_bfs;
    Graph &g;                                       ///< 处理的图。
    PartitionManager partition_manager_;            ///< 分区管理器。
    std::unique_ptr<GraphPartitioner> partitioner_; ///< 图分区器，支持多种分区算法。
    //BloomFilter bloom_filter_;                      ///< Bloom Filter，用于快速判断节点间可能的连接。
    NodeEmbedding node_embedding_;                  ///< 节点嵌入，用于压缩邻接矩阵。
    std::string partitioner_name_;
    unordered_map<size_t, vector<vector<size_t>>> unreachable_index_;          ///< 不可达分区索引，存储分区内的不可达点对的邻接表。
    unordered_map<size_t, unordered_map<size_t, size_t>> unreachable_mapping_; ///< 不可达分区里面的点的映射关系，不连续点到连续的数据结构的映射关系
    unordered_map<size_t, vector<vector<bitset<1>>>> small_index_;             ///< 小分区索引,存储可达点对的邻接矩阵。
    unordered_map<size_t, unordered_map<size_t, size_t>> small_mapping_;       ///< 小分区分区里面点的映射关系，不连续点到连续的数据结构的映射关系
    std::unordered_map<size_t, PLL *> pll_index_;                              ///< PLL索引，用于存储每个节点的PLL索引。
    bool is_index;                                                             ///< 是否使用索引
};
#endif // COMPRESSED_SEARCH_H
