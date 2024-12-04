#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "graph.h"

//TODO: mapping 在分区变化的函数中没有正确维护，仅在建立分区图的时候有初始化


// 表示两个分区之间的边
struct PartitionEdge {
    std::vector<std::pair<int, int>> original_edges; // 原始图中的边 <source, target>
    int edge_count = 0; // 边的数量
};

class PartitionManager {
public:
    PartitionManager(Graph &graph);

    // 建立分区图
    void build_partition_graph();
    // 设置节点的分区ID
    void set_partition(int node, int partitionId);
    int get_partition(int node) const {return g.get_partition_id(node);}

    // 获取分区的所有连接
    const std::unordered_map<int, PartitionEdge>& getPartitionConnections(int partitionId) const;
    // 获取两个分区之间的连接
    const PartitionEdge& get_partition_adjacency(int u, int v) const;


    // 获取指定分区的顶点集合
    const std::unordered_set<int>& get_vertices_in_partition(int partition_id) const {
        auto it = mapping.find(partition_id);
        if (it != mapping.end()) {
            return it->second;
        }
        static const std::unordered_set<int> empty_set;
        return empty_set;
    }

    
    // 获取图对象
    const Graph& get_graph() const {
        return g;
    }

    // 移除分区
    void remove_partition(int partition_id) {
        mapping.erase(partition_id);
    }

    // 更新分区之间的连接
    void update_partition_connections();


    // 获取分区和点的映射关系
    const std::unordered_map<int, std::unordered_set<int>>& get_mapping() const {
        return mapping;
    }


    // 分区之间的邻接表
    std::unordered_map<int, std::unordered_map<int, PartitionEdge>> partition_adjacency;

    // 分区和点的映射关系
    std::unordered_map<int, std::unordered_set<int>> mapping;
    Graph &g;
    Graph part_g;

<<<<<<< HEAD
    // 分区ID到子图的映射
    std::unordered_map<int, Graph> partition_subgraphs;
=======
>>>>>>> 6e9f9388b318dd14efce6e7e30b3f294f68e4179
        // 添加缓存：社区的入度、出度和内部边数
    std::unordered_map<int, double> community_in_degree;
    std::unordered_map<int, double> community_out_degree;
    std::unordered_map<int, double> community_internal_edges;

    // 更新社区的统计信息
    void update_community_stats(int community_id, double in_degree_delta, double out_degree_delta, double internal_edge_delta) {
        community_in_degree[community_id] += in_degree_delta;
        community_out_degree[community_id] += out_degree_delta;
        community_internal_edges[community_id] += internal_edge_delta;
    }

    // 获取社区的统计信息
    double get_community_in_degree(int community_id) const {
        return community_in_degree.at(community_id);
    }

    double get_community_out_degree(int community_id) const {
        return community_out_degree.at(community_id);
    }

    double get_community_internal_edges(int community_id) const {
        return community_internal_edges.at(community_id);
    }




private:

};

#endif // PARTITION_MANAGER_H