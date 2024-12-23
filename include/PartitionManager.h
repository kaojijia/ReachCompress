#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include <utility>
#include <iostream>
#include "graph.h"
#include "CSR.h"
// 自定义哈希函数和相等比较函数，用于 std::unordered_set
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // 组合哈希值，减少碰撞
    }
};

struct pair_equal {
    template <class T1, class T2>
    bool operator() (const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2) const {
        return p1.first == p2.first && p1.second == p2.second;
    }
};

// 表示两个分区之间的边
struct PartitionEdge
{
    std::unordered_set<std::pair<int, int>, pair_hash, pair_equal> original_edges; // 原始图中的边 <source, target>
    int edge_count = 0;                                                            // 边的数量

    // 插入边
    void add_edge(int u, int v) {
        auto result = original_edges.emplace(u, v);
        if (result.second) {
            edge_count++;
        }
    }

    // 删除边
    void remove_edge(int u, int v) {
        auto it = original_edges.find(std::make_pair(u, v));
        if (it != original_edges.end()) {
            original_edges.erase(it);
            edge_count--;
        }
    }
};

class PartitionManager
{
public:
    PartitionManager(Graph &graph);

    // void build_partition_graph_with_CSRs();

    // 建立分区图
    void build_partition_graph();
    void build_subgraphs();
    void build_partition_graph_without_subgraph();

    // 更新 顶点的分区id、分区图、分区之间的连接、分区和顶点的映射
    // 好像 没有分区子图
    void update_partition_info(int node, int old_partition_id, int new_partition_id);

    
    // 更新分区之间的连接
    void update_partition_connections();

    // void update_partition_connections_CSR();

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

    int get_partition_id(int node) const {
        return csr->getPartition(node); 
        // return g.get_partition_id(node); 
    }

    // 获取分区的所有连接
    std::map<int, PartitionEdge> get_partition_adjacency(int partitionId);
    // 获取两个分区之间的连接
    PartitionEdge get_partition_adjacency(int u, int v);

    // 获取指定分区的顶点集合
    const std::set<int> &get_vertices_in_partition(int partition_id) const
    {
        auto it = mapping.find(partition_id);
        if (it != mapping.end())
        {
            return it->second;
        }
        static const std::set<int> empty_set;
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


    void print_equivalence_mapping() const;

    // 从文件中读取节点的分区信息，更新到图中，有些从边集没有读取到的边会在这里补全
    void read_equivalance_info(const std::string &filename);

    uint32_t get_equivalance_mapping(int node) const{
        uint32_t eq = *equivalence_mapping[node];
        return ((eq==4156623648)?static_cast<uint32_t>(node):eq);
    };

    uint32_t get_equivalence_mapping_size() const
    {
        uint32_t total_size = 0;
        uint32_t num_vertices = csr->max_node_id+1;
        for (uint32_t i = 0; i < num_vertices; ++i)
        {
            if (equivalence_mapping[i] != nullptr)
            {
                total_size +=  sizeof(uint32_t);
                
            }
            total_size +=  sizeof(uint32_t);
        }
        return total_size;
    }

    // 获取分区和点的映射关系
    const std::map<int, std::set<int>> &get_mapping() const
    {
        return mapping;
    }

    // 分区之间的邻接表
    std::map<int, std::map<int, PartitionEdge>> partition_adjacency;

    // 分区和点的映射关系,第一位是分区号，第二位是顶点
    std::map<int, std::set<int>> mapping;
    Graph &g;
    CSRGraph *csr;
    Graph part_g;
    CSRGraph *part_csr;

    // 分区ID到子图的映射
    std::unordered_map<int, Graph> partition_subgraphs;
    std::map<uint32_t, std::shared_ptr<CSRGraph>> partition_subgraphs_csr;

    // 等价类的二维数组映射,第一位是原始节点，第二位是映射后的节点。在g中的vertex结构体里也有这个
    uint32_t **equivalence_mapping;

private:

};

#endif // PARTITION_MANAGER_H