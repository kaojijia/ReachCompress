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
    SetSearch(Graph &g);
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

    //拓扑层级
    vector<int> topo_level;

    // 树索引
    shared_ptr<TreeCover> tree_cover;


    //关键路标点索引构建
    int num_key_points = 2;
    void build_key_points(int num);
    //拓扑索引
    void build_topo_level();
    //生成树索引
    // void build_tree_index();



    // 集合森林节点
    struct ForestNode {
        int id;
        int depth;
        std::unordered_set<int> covered_nodes; // 覆盖的原始节点
        std::vector<std::shared_ptr<ForestNode>> children;
        std::weak_ptr<ForestNode> parent;
        std::vector<int> key_points; // 缓存该节点代表的关键点
        ForestNode(int id) : id(id), depth(0) {}
    };

    using ForestNodePtr = std::shared_ptr<ForestNode>;
    
    // 优先队列比较器
    struct QueueItem {
        SetSearch::ForestNodePtr s_node;
        SetSearch::ForestNodePtr t_node;
    };


    // 森林构建函数
    vector<ForestNodePtr>build_forest(const set<int>& nodes, bool is_source);
    vector<ForestNodePtr> build_source_forest(const set<int>& nodes);
    vector<ForestNodePtr> build_target_forest(const set<int>& targets);
    //维护到根节点的连接
    void establish_parent_links(ForestNodePtr node);
    // 找共同关键点
    int find_common_keypoint(const set<int> &kps1,const set<int> &kps2);
};

