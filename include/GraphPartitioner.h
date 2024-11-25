#ifndef GRAPH_PARTITIONER_H
#define GRAPH_PARTITIONER_H

#include "graph.h"
#include "PartitionManager.h"

/**
 * @class GraphPartitioner
 * @brief 抽象基类，用于实现不同的图分区算法。
 */
class GraphPartitioner {
public:
    /**
     * @brief 虚析构函数。
     */
    virtual ~GraphPartitioner() = default;

    /**
     * @brief 执行图分区算法。
     * @param graph 需要分区的图。
     * @param partition_manager 用于管理和存储分区信息的管理器。
     */
    virtual void partition(Graph& graph, PartitionManager& partition_manager) = 0;
};

#endif // GRAPH_PARTITIONER_H