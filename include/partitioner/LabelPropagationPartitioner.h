#pragma once


#include "GraphPartitioner.h"


class LabelPropagationPartitioner :public GraphPartitioner{

public:
    void partition(Graph& graph, PartitionManager& partition_manager) override;

    ~LabelPropagationPartitioner() override = default;
private:    


};