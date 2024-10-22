#ifndef GRAPH_PRUNER_H
#define GRAPH_PRUNER_H

#include "graph.h"

class GraphPruner {
public:
    GraphPruner(Graph& graph);  // 构造函数
    void pruneIsolatedNodes();  // 裁剪孤立节点
    void pruneByDegree(int min_in_degree, int min_out_degree);  // 根据度数裁剪
    void pruneByNodeSet(const std::unordered_set<int>& node_set);  // 裁剪指定节点集合

private:
    Graph& g;  // 图的引用
};

#endif
