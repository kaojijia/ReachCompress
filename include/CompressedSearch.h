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
#include "partitioner/GraphPartitioner.h"
#include "partitioner/ReachRatioPartitioner.h"
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include "BidirectionalBFS.h"
#include "BiBFSCSR.h"
#include "TreeCover.h"

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
          is_index(is_index),
          filter_name_("")
    {
        set_partitioner(partitioner_name);
    }

    // 设置分区器
    void set_partitioner(std::string partitioner_name);

    void offline_industry() override;

    void offline_industry(size_t num_vertices, float ratio, string mapping_file);


    
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
        index_sizes["Equivalence Mapping"] = 0;
        index_sizes["G'CSR"] = 0;
        index_sizes["Partition id"] = g.vertices.size() * sizeof(uint16_t);
        index_sizes["Partition Connection"] = 0;
        index_sizes["PLL_in_pointers"] = 0;
        index_sizes["PLL_out_pointers"] = 0;
        index_sizes["PLL_in_sets"] = 0;
        index_sizes["PLL_out_sets"] = 0;
        index_sizes["Reachable Matrix"] = 0;
        index_sizes["Unreachable Index"] = 0;
        index_sizes["Total"] = 0;


        // 计算 PLL 索引的大小
        // for (const auto &pll : pll_index_)
        // {
        //     auto i = pll.second->getIndexSizes();
        //     index_sizes["PLL"] += i["IN"] + i["OUT"];
        // }
        // 计算 PLL 索引的大小
        for (const auto &pll : pll_index_) {
            auto i = pll.second->getIndexSizes();
            index_sizes["PLL_in_pointers"] += i["in_pointers"];
            index_sizes["PLL_out_pointers"] += i["out_pointers"];
            index_sizes["PLL_in_sets"] += i["in_sets"];
            index_sizes["PLL_out_sets"] += i["out_sets"];
        }

        // 计算小分区索引的大小
        for (const auto &small_index : small_index_)
        {
            int a = small_index.second.size();
            index_sizes["Reachable Matrix"] += a*a;
        }

        // 计算不可达索引的大小
        for (const auto &unreachable_index : unreachable_index_)
        {
            for (const auto &row : unreachable_index.second)
            {
                index_sizes["Unreachable adjList"] += row.size() * sizeof(size_t);
            }
        }

        // 计算分区图的大小
        index_sizes["G'CSR"] += partition_manager_.part_csr->getMemoryUsage();

        // 计算分区信息的大小
        index_sizes["Partition Mapping"] += partition_manager_.mapping.size() * sizeof(std::unordered_set<int>);

        // 计算分区间的联系的大小
        index_sizes["Partition Connection"] += partition_manager_.partition_adjacency.size() * sizeof(std::unordered_map<int, PartitionEdge>);

        // 计算等价类大小
        index_sizes["Equivalence Mapping"] += partition_manager_.get_equivalence_mapping_size();

        uint32_t total = 0;
        for(auto i : index_sizes){
            total += i.second;
        }
        index_sizes["Total"] = total;

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
    void partition_graph(); ///< 图分区算法
    bool query_within_partition(int source, int target); ///< 同分区查询
    bool query_index_within_partition(int source, int target, int partition_id); ///< 同分区查询
    bool query_across_partitions(int source, int target); ///< 跨分区查询
    bool query_across_partitions_with_all_paths(int source, int target);
    bool dfs_paths_search(int current_partition, int target_partition, std::vector<int> &path, std::vector<std::vector<int>> &all_paths, std::unordered_set<int> &visited, int source, int target);
    bool dfs_partition_search(int u, std::vector<std::pair<int, int>> edges, std::vector<int> path, int target);
    void build_partition_index(float ratio, size_t num_vertices); ///< 构建分区索引
    void construct_filter(float ratio);
    
    // std::unique_ptr<BidirectionalBFS> part_bfs;           ///< 分区图上的双向BFS类。
    std::unique_ptr<Algorithm> filter;              ///< 过滤器，看a来实现
    std::string filter_name_;
    std::unique_ptr<BidirectionalBFS> part_bfs;
    std::unique_ptr<BiBFSCSR> part_bfs_csr;
    Graph &g;                                       ///< 处理的图。
    PartitionManager partition_manager_;            ///< 分区管理器。
    std::unique_ptr<GraphPartitioner> partitioner_; ///< 图分区器，支持多种分区算法。
    // BloomFilter bloom_filter_;                      ///< Bloom Filter，用于快速判断节点间可能的连接。
    // NodeEmbedding node_embedding_; ///< 节点嵌入，用于压缩邻接矩阵。
    std::string partitioner_name_;
    unordered_map<size_t, vector<vector<size_t>>> unreachable_index_;          ///< 不可达分区索引，存储分区内的不可达点对的邻接表。
    unordered_map<size_t, unordered_map<size_t, size_t>> unreachable_mapping_; ///< 不可达分区里面的点的映射关系，不连续点到连续的数据结构的映射关系
    unordered_map<size_t, vector<vector<bitset<1>>>> small_index_;             ///< 小分区索引,存储可达点对的邻接矩阵。
    unordered_map<size_t, unordered_map<size_t, size_t>> small_mapping_;       ///< 小分区分区里面点的映射关系，不连续点到连续的数据结构的映射关系
    std::unordered_map<size_t, PLL *> pll_index_;                              ///< PLL索引，用于存储每个节点的PLL索引。
    bool is_index;                                                             ///< 是否使用索引
};
#endif // COMPRESSED_SEARCH_H
