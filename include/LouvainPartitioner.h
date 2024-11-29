#ifndef LOUVAIN_PARTITIONER_H
#define LOUVAIN_PARTITIONER_H

#include "GraphPartitioner.h"

/**
 * @class LouvainPartitioner
 * @brief 使用 Louvain 算法进行图分区的具体实现类。
 */
class LouvainPartitioner : public GraphPartitioner {
public:
    double compute_gain(int node, int target_partition, Graph &graph, PartitionManager &partition_manager, double m);
    /**
     * @brief 执行 Louvain 分区算法。
     * @param graph 需要分区的图。
     * @param partition_manager 用于管理和存储分区信息的管理器。
     */
    void partition(Graph& graph, PartitionManager& partition_manager) override;
};

#endif // LOUVAIN_PARTITIONER_H