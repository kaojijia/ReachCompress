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
#include "partitioner/RandomPartitioner.h"
#include "partitioner/MultiCutPartitioner.h"
#include "BloomFilter.h"
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
          csr(nullptr),
          part_bfs(nullptr),
          is_index(is_index),
          filter_name_("")
    {
        set_partitioner(partitioner_name);
        this->csr = partition_manager_.csr;
    }
    ~CompressedSearch() override
    {
        for (auto &pll : pll_index_)
        {
            delete pll.second;
        }
        delete csr;
    }

    // 设置分区器
    void set_partitioner(std::string partitioner_name);

    void offline_industry() override;

    void offline_industry(size_t num_vertices, float ratio, string mapping_file = "");

    bool reachability_query(int source, int target) override;

    std::vector<std::string> get_index_info();

    PartitionManager &get_partition_manager()
    {
        return partition_manager_;
    };

    BidirectionalBFS bfs; ///< 原图上的双向BFS算法。

std::vector<std::pair<std::string, std::string>> getIndexSizes() const override
{
    std::vector<std::pair<std::string, std::string>> index_sizes;
    index_sizes.emplace_back("Equivalence Mapping", "0");
    index_sizes.emplace_back("G'CSR", "0");
    index_sizes.emplace_back("Partition id", std::to_string(g.vertices.size() * sizeof(uint16_t)));
    index_sizes.emplace_back("Partition Connection", "0");
    index_sizes.emplace_back("PLL_in_pointers", "0");
    index_sizes.emplace_back("PLL_out_pointers", "0");
    index_sizes.emplace_back("PLL_in_sets", "0");
    index_sizes.emplace_back("PLL_out_sets", "0");
    index_sizes.emplace_back("Reachable Matrix", "0");
    index_sizes.emplace_back("Unreachable Index", "0");
    index_sizes.emplace_back("Total", "0");

    // 计算 PLL 索引的大小
    for (const auto &pll : pll_index_) {
        auto pll_sizes = pll.second->getIndexSizes();
        for (const auto &size : pll_sizes) {
            if (size.first == "in_pointers") {
                index_sizes[4].second = std::to_string(std::stoull(index_sizes[4].second) + std::stoull(size.second));
            } else if (size.first == "out_pointers") {
                index_sizes[5].second = std::to_string(std::stoull(index_sizes[5].second) + std::stoull(size.second));
            } else if (size.first == "in_sets") {
                index_sizes[6].second = std::to_string(std::stoull(index_sizes[6].second) + std::stoull(size.second));
            } else if (size.first == "out_sets") {
                index_sizes[7].second = std::to_string(std::stoull(index_sizes[7].second) + std::stoull(size.second));
            }
        }
    }

    // 计算小分区索引的大小
    for (const auto &small_index : small_index_) {
        int a = small_index.second.size();
        index_sizes[8].second = std::to_string(std::stoull(index_sizes[8].second) + a * a);
    }

    // 计算不可达索引的大小
    for (const auto &unreachable_index : unreachable_index_) {
        for (const auto &row : unreachable_index.second) {
            index_sizes[9].second = std::to_string(std::stoull(index_sizes[9].second) + row.size() * sizeof(size_t));
        }
    }

    // 计算分区图的大小
    index_sizes[1].second = std::to_string(std::stoull(index_sizes[1].second) + partition_manager_.part_csr->getMemoryUsage());

    // 计算分区信息的大小
    index_sizes.emplace_back("Partition Mapping", std::to_string(partition_manager_.mapping.size() * sizeof(std::unordered_set<int>)));

    // 计算分区间的联系的大小和边的数量
    size_t partition_connection_size = 0;
    size_t partition_connection_memory = 0;
    size_t total_edges = 0;
    for (const auto &outer_pair : partition_manager_.partition_adjacency) {
        for (const auto &inner_pair : outer_pair.second) {
            partition_connection_size++;
            partition_connection_memory += inner_pair.second.original_edges.size() * sizeof(std::pair<int, int>);
            partition_connection_memory += sizeof(inner_pair.second.edge_count);
            total_edges += inner_pair.second.original_edges.size();
        }
    }
    index_sizes.emplace(index_sizes.begin() + 3, "Total Edges in Partition Connections", std::to_string(total_edges));
    index_sizes[4].second = std::to_string(partition_connection_memory);

    // 计算等价类大小
    if (partition_manager_.equivalence_mapping != nullptr) {
        index_sizes[0].second = std::to_string(std::stoull(index_sizes[0].second) + partition_manager_.get_equivalence_mapping_size());
    }

    uint64_t total = 0;
    for (const auto &i : index_sizes) {
        total += std::stoull(i.second);
    }
    index_sizes[11].second = std::to_string(total);

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
    void partition_graph();                                                      ///< 图分区算法
    bool query_within_partition(int source, int target);                         ///< 同分区查询
    bool query_index_within_partition(int source, int target, int partition_id); ///< 同分区查询
    bool query_across_partitions(int source, int target);                        ///< 跨分区查询
    bool query_across_partitions_with_all_paths(int source, int target);
    bool dfs_paths_search(int current_partition, int target_partition, std::vector<int> &path, std::vector<std::vector<int>> &all_paths, std::unordered_set<int> &visited, int source, int target);
    bool dfs_partition_search(int u, std::vector<std::pair<int, int>> edges, std::vector<int> path, int target);
    void build_partition_index(float ratio, size_t num_vertices); ///< 构建分区索引
    void construct_filter(float ratio);

    // std::unique_ptr<BidirectionalBFS> part_bfs;           ///< 分区图上的双向BFS类。
    std::unique_ptr<Algorithm> filter; ///< 过滤器，看a来实现
    std::string filter_name_;
    std::unique_ptr<BidirectionalBFS> part_bfs;
    std::unique_ptr<BiBFSCSR> part_bfs_csr;
    Graph &g; ///< 处理的图。
    CSRGraph *csr;
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
