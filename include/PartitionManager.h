
#ifndef PARTITION_MANAGER_H
#define PARTITION_MANAGER_H

#include <unordered_map>

class PartitionManager {
public:
    // 设置节点的分区ID
    void set_partition(int node, int partitionId);
    // 获取节点的分区ID
    int get_partition(int node) const;
    // 判断两个节点是否在同一分区
    bool is_same_partition(int node1, int node2) const;

private:
    std::unordered_map<int, int> node_partition;  // 节点到分区ID的映射
};

#endif // PARTITION_MANAGER_H