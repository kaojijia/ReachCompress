#pragma once

#include "GraphPartitioner.h"


class MultiCutPartitioner: public GraphPartitioner {
public:
    void partition(Graph& graph, PartitionManager& partition_manager) override;
    ~MultiCutPartitioner(){};
};