#ifndef PLL_H
#define PLL_H

#include "graph.h"
#include <vector>

class PLL {
public:
    PLL(Graph& graph);  // 构造函数，传入图的引用

    // 构建PLL标签
    void buildPLLLabels();

    // 可达性查询
    bool reachabilityQuery(int u, int v);

    // 贪婪算法用于2-Hop覆盖
    std::vector<int> greedy2HopCover();

private:
    Graph& g;  // 引用图
    void bfsPruned(int start, bool is_reversed);  // 剪枝的BFS，用于标签构建
};

#endif
