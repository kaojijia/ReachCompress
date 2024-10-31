#ifndef PLL_H
#define PLL_H

#include "graph.h"
#include <vector>

class PLL {
public:
    PLL(Graph& graph);  // 构造函数，传入图的引用

    // 构建PLL标签
    void buildPLLLabels();
    // 完全体标签
    void buildPLLLabelsUnpruned();
    // 可达性查询
    bool query(int u, int v);

    std::vector<std::vector<int>> IN;
    std::vector<std::vector<int>> OUT;

private:
    Graph& g;  // 引用图

    std::vector<std::vector<int>> adjList;         // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList;  // 逆邻接表

    void buildAdjList();
    void buildInOut();
    
    //2hop可达性查询，用于剪枝
    bool HopQuery(int u, int v);

    //IN和OUT集合去重
    void simplifyInOutSets();
    
    void bfsUnpruned(int start, bool is_reversed);
    void bfsPruned(int start);

    //确定剪枝顺序    
    std::vector<int> orderByDegree();
};

#endif
