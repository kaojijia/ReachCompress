#pragma once
#include "GraphPartitioner.h"


class RandomPartitioner : public GraphPartitioner {

public:
    void partition(Graph& graph, PartitionManager& partition_manager) override;


private:
    uint32_t num_partitions = 100;


};