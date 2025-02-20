#pragma once

#include <sstream>
#include <bitset>
#include <memory>
#include <unordered_map>
#include <list>
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
    SetSearch(Graph &g, int num_key_points=5);
    ~SetSearch() override;
    vector<pair<int, int>> set_reachability_query(vector<int> source_set, vector<int> target_set);
    bool reachability_query(int source, int target) override;
    void offline_industry() override;
    std::vector<std::pair<std::string, std::string>> getIndexSizes() const override;

    shared_ptr<PLL> pll;
private:

    shared_ptr<Graph> g;
    shared_ptr<CSRGraph> csr;


    //关键路标点
    vector<set<int>> in_key_points;
    vector<set<int>> out_key_points;
    std::vector<int> key_points;

    //拓扑层级
    vector<int> topo_level;


    // 树索引
    shared_ptr<TreeCover> tree_cover;


    //关键路标点索引构建
    int num_key_points = 5;
    void build_key_points(int num);
    //拓扑索引
    void build_topo_level();
    void build_topo_level_optimized();
    // 生成树索引
    //  void build_tree_index();

    // 集合森林节点
    struct ForestNode {
        int id;
        std::unordered_set<int> covered_nodes; // 覆盖的原始节点
        std::vector<std::shared_ptr<ForestNode>> children; // 当前的孩子节点
        std::weak_ptr<ForestNode> parent; // 当前的父节点
        ForestNode(int id) : id(id) {}
    };

    using ForestNodePtr = std::shared_ptr<ForestNode>;


    // 森林构建函数
    vector<ForestNodePtr> build_forest(const set<int>& nodes, bool is_source);
    vector<ForestNodePtr> build_source_forest(const set<int>& nodes);
    vector<ForestNodePtr> build_target_forest(const set<int>& targets);
    vector<ForestNodePtr> build_forest_optimized(const set<int>& nodes, bool is_source);
    //维护到根节点的连接
    void establish_parent_links(ForestNodePtr node);
    // 找共同关键点
    int find_common_keypoint(const set<int> &kps1,const set<int> &kps2);
};

