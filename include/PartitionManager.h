#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "graph.h"

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


    // 更新分区之间的连接
    void update_partition_connections();

    // 分区之间的邻接表
    std::unordered_map<int, std::unordered_map<int, PartitionEdge>> partition_adjacency;

    // 分区和点的映射关系
    std::unordered_map<int, std::unordered_set<int>> mapping;
    Graph &g;
    Graph part_g;
private:

};

#endif // PARTITION_MANAGER_H