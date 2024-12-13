#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include "graph.h"
#include "CSR.h"

// TODO: mapping 在分区变化的函数中没有正确维护，仅在建立分区图的时候有初始化

// 表示两个分区之间的边
struct PartitionEdge
{
    std::vector<std::pair<int, int>> original_edges; // 原始图中的边 <source, target>
    int edge_count = 0;                              // 边的数量
};

class PartitionManager
{
public:
    PartitionManager(Graph &graph);

    // 建立分区图
    void build_partition_graph();

    // 设置节点的分区ID
    void set_partition(int node, int partitionId)
    {
        int old_partition = g.get_partition_id(node);
        if (old_partition != partitionId)
        {
            mapping[old_partition].erase(node);
        }
        g.set_partition_id(node, partitionId);
        mapping[partitionId].insert(node);
    };
    int get_partition(int node) const { return g.get_partition_id(node); }

    // 获取分区的所有连接
    const std::unordered_map<int, PartitionEdge> &getPartitionConnections(int partitionId) const;
    // 获取两个分区之间的连接
    const PartitionEdge &get_partition_adjacency(int u, int v) const;

    // 获取指定分区的顶点集合
    const std::unordered_set<int> &get_vertices_in_partition(int partition_id) const
    {
        auto it = mapping.find(partition_id);
        if (it != mapping.end())
        {
            return it->second;
        }
        static const std::unordered_set<int> empty_set;
        return empty_set;
    }

    // 获取图对象
    const Graph &get_graph() const
    {
        return g;
    }

    // 移除分区
    void remove_partition(int partition_id)
    {
        mapping.erase(partition_id);
    }

    // 更新分区之间的连接
    void update_partition_connections();

    void print_equivalence_mapping() const;

    // 从文件中读取节点的分区信息，更新到图中，有些从边集没有读取到的边会在这里补全
    void read_equivalance_info(const std::string &filename);

    uint32_t get_equivalence_mapping_size() const
    {
        uint32_t total_size = 0;
        uint32_t num_vertices = csr->max_node_id+1;
        for (uint32_t i = 0; i < num_vertices; ++i)
        {
            if (equivalence_mapping[i] != nullptr)
            {
                // 假设每行的第一个元素是行的长度
                uint32_t row_size = equivalence_mapping[i][0];
                total_size += row_size * sizeof(uint32_t);
            }
        }
        return total_size;
    }

    // 获取分区和点的映射关系
    const std::unordered_map<int, std::unordered_set<int>> &get_mapping() const
    {
        return mapping;
    }

    // 分区之间的邻接表
    std::unordered_map<int, std::unordered_map<int, PartitionEdge>> partition_adjacency;

    // 分区和点的映射关系
    std::unordered_map<int, std::unordered_set<int>> mapping;
    Graph &g;
    Graph part_g;

    // 分区ID到子图的映射
    std::unordered_map<int, Graph> partition_subgraphs;
    std::map<uint32_t, std::shared_ptr<CSRGraph>> partition_subgraphs_csr;

    // 等价类的二维数组映射
    uint32_t **equivalence_mapping;

private:
    CSRGraph *csr;
};

#endif // PARTITION_MANAGER_H