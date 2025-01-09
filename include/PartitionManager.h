#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include "graph.h"
#include "CSR.h"
#include "Algorithm.h"

using namespace std;
//分区的出口和入口点集
struct ConnectNode
{
    vector<int> outgoing_nodes;
    vector<int> incoming_nodes;
    void add_incoming_node(int node)
    {
        auto it = std::lower_bound(incoming_nodes.begin(), incoming_nodes.end(), node);
        if (it == incoming_nodes.end() || *it != node)
        {
            incoming_nodes.insert(it, node);
        }
    }

    void add_outgoing_node(int node)
    {
        auto it = std::lower_bound(outgoing_nodes.begin(), outgoing_nodes.end(), node);
        if (it == outgoing_nodes.end() || *it != node)
        {
            outgoing_nodes.insert(it, node);
        }
    }
};

// 表示两个分区之间的边
struct PartitionEdge
{
    std::vector<std::pair<int, int>> original_edges; // 原始图中的边 <source, target>
    int edge_count = 0;                              // 边的数量
    // 插入边时保持 original_edges 有序
    void add_edge(int u, int v)
    {
        auto it = std::lower_bound(original_edges.begin(), original_edges.end(), std::make_pair(u, v));
        if (it == original_edges.end() || *it != std::make_pair(u, v))
        {
            original_edges.emplace(it, u, v);
            edge_count++;
        }
    }

    // 删除边
    void remove_edge(int u, int v)
    {
        auto it = std::lower_bound(original_edges.begin(), original_edges.end(), std::make_pair(u, v));
        if (it != original_edges.end() && *it == std::make_pair(u, v))
        {
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
    // 后两个合并起来是上面那个函数
    void build_subgraphs();
    void build_partition_graph_without_subgraph();

    // 根据输入的分区号更新 顶点的分区id、分区图、分区之间的连接、分区和顶点的映射
    // 不更新 分区子图adj和分区连接图connect_g
    void update_partition_info(int node, int old_partition_id, int new_partition_id);

    // 更新分区之间的连接partition_adj
    void update_partition_connections();

    // 根据partition_adj创建分区连接图part_connect_g，
    // 以及每个分区的出口点集和入口点集
    // 后面根据这个图创建索引
    // 如果已有会删除
    void build_connections_graph();


    void remove_partition(int partition_id) { mapping.erase(partition_id); }



//=============================================================================================
// getter 方法 setter 方法 和辅助方法
    const Graph &get_graph() const { return g; }

    void print_equivalence_mapping() const;

    // 从文件中读取节点的分区信息，更新到图中，有些从边集没有读取到的边会在这里补全
    void read_equivalance_info(const std::string &filename);

    uint32_t get_equivalance_mapping(int node) const
    {
        if (equivalence_mapping == nullptr)
            return node;
        uint32_t eq = *equivalence_mapping[node];
        return ((eq == 999999999) ? static_cast<uint32_t>(node) : eq);
    };

    uint32_t get_equivalence_mapping_size() const
    {

        uint32_t total_size = 0;
        uint32_t num_vertices = csr->max_node_id + 1;
        for (uint32_t i = 0; i < num_vertices; ++i)
        {
            if (equivalence_mapping[i] != nullptr)
            {
                total_size += sizeof(uint32_t);
            }
            total_size += sizeof(uint32_t);
        }
        return total_size;
    }
    const std::map<int, std::set<int>> &get_mapping() const{return mapping;}

    // 获取两个分区之间的连接
    PartitionEdge get_partition_adjacency(int u, int v);

    void save_mapping(const std::string &filename) const;

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

    int get_partition_id(int node) const
    {
        return csr->getPartition(node);
        // return g.get_partition_id(node);
    }

    // 获取分区的所有连接
    std::unordered_map<int, PartitionEdge> get_partition_adjacency(int partitionId);
    // 获取分区的连接数
    std::pair<int, int> get_partition_adjacency_size(int partitionId) const
    {
        int count = 0;
        int single = 0;
        auto it = partition_adjacency.find(partitionId);
        if (it == partition_adjacency.end())
        {
            return {0, 0};
        }
        for (const auto &edge : it->second)
        {
            int num = edge.second.edge_count;
            if (num >= 6)
                single = 1;
            count += num;
        }
        return {count, single};
    }
//=============================================================================================




    // 分区和点的映射关系,第一位是分区号，第二位是顶点
    std::map<int, std::set<int>> mapping;
    // 原图
    Graph &g;
    CSRGraph *csr;
    // 分区图
    Graph part_g;
    CSRGraph *part_csr;

    // 分区出入口集合的图
    std::shared_ptr<Graph> part_connect_g;
    std::shared_ptr<CSRGraph> part_connect_csr;
    // 每个分区出口和入口点集
    unordered_map<int, unordered_map<int,ConnectNode>> connect_nodes;
    // 分区之间的邻接表
    std::unordered_map<int, std::unordered_map<int, PartitionEdge>> partition_adjacency;

    // 每个分区对应的子图
    std::unordered_map<int, Graph> partition_subgraphs;
    std::map<uint32_t, std::shared_ptr<CSRGraph>> partition_subgraphs_csr;

    // 等价类的二维数组映射,第一位是原始节点，第二位是映射后的节点。在g中的vertex结构体里也有这个
    uint32_t **equivalence_mapping = nullptr;

private:
    std::string getCurrentTimes()
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
        tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_tm, &now_time_t);
#else
        localtime_r(&now_time_t, &local_tm);
#endif

        std::stringstream ss;
        ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
        return ss.str();
    }
};

#endif // PARTITION_MANAGER_H