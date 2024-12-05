#ifndef INFOMAP_PARTITIONER_H
#define INFOMAP_PARTITIONER_H

#include "GraphPartitioner.h"

/**
 * @class InfomapPartitioner
 * @brief 使用 Infomap 算法进行图分区的具体实现类。
 */
class InfomapPartitioner : public GraphPartitioner {
public:
    /**
     * @brief 执行 Infomap 分区算法。
     * @param graph 需要分区的图。
     * @param partition_manager 用于管理和存储分区信息的管理器。
     */
    void partition(Graph& graph, PartitionManager& partition_manager) override;
};

#endif // INFORMAP_PARTITIONER_H