#include "PartitionManager.h"

/**
 * @brief 设置节点的分区ID。
 * @param node 节点编号。
 * @param partitionId 分区ID。
 */
void PartitionManager::set_partition(int node, int partition_id) {
    node_partition[node] = partition_id;
}

/**
 * @brief 获取节点的分区ID。
 * @param node 节点编号。
 * @return 分区ID，如果节点未分配分区则返回 -1。
 */
int PartitionManager::get_partition(int node) const {
    auto it = node_partition.find(node);
    if (it != node_partition.end()) {
        return it->second;
    } else {
        return -1;  // 节点未分配分区时返回 -1
    }
}

/**
 * @brief 判断两个节点是否在同一分区。
 * @param node1 第一个节点编号。
 * @param node2 第二个节点编号。
 * @return 如果在同一分区返回 true，否则返回 false。
 */
bool PartitionManager::is_same_partition(int node1, int node2) const {
    return get_partition(node1) == get_partition(node2);
}