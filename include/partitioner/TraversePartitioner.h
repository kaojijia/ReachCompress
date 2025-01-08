#pragma once
#include "GraphPartitioner.h"
#include "graph.h"
#include "CSR.h"
#include "BiBFSCSR.h"
#include "ReachRatioPartitioner.h" 
#include <iostream>
#include <vector>
#include <stack>
#include <utility>
#include <unordered_set>
#include <bitset>
#define u32 uint32_t



class TraversePartitioner : public GraphPartitioner
{
public:
    TraversePartitioner() : bibfs_(nullptr), csr_(nullptr){
    }
    void partition(Graph& graph, PartitionManager& partition_manager) override;

    vector<u32> dfs(u32 depth, u32 vertex, map<u32, PartitionReachable> *partition_reachable = nullptr);

    ~TraversePartitioner() override = default;


private:
    vector<u32> partitions;
    shared_ptr<CSRGraph> csr_;
    shared_ptr<BiBFSCSR> bibfs_;

};