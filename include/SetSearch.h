#pragma once

#include <sstream>
#include <bitset>
#include <memory>
#include <unordered_map>
#include "pll.h"
#include "graph.h"
#include "Algorithm.h"
#include "PartitionManager.h"
#include "BloomFilter.h"
#include "BidirectionalBFS.h"
#include "BiBFSCSR.h"
#include "TreeCover.h"


using namespace std;

class SetSearch : public Algorithm {

public:
    SetSearch(Graph &g);
    ~SetSearch() override;
    vector<pair<int, int>> set_reachability_query(vector<int> source_set, vector<int> target_set);
    bool reachability_query(int source, int target) override;
    void offline_industry() override;

private:

    shared_ptr<Graph> g;
    shared_ptr<CSRGraph> csr;
    unique_ptr<PLL> pll;

    //关键路标点
    vector<set<int>> in_key_points;
    vector<set<int>> out_key_points;

    //拓扑层级
    vector<int> topo_level;


    //关键路标点索引构建
    int num_key_points = 10;
    void build_key_points(int num);
    //拓扑索引
    void build_topo_level();
    //生成树索引
    void build_tree_index();
};

