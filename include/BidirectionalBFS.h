#ifndef BIDIRECTIONAL_BFS_H
#define BIDIRECTIONAL_BFS_H

#include "graph.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <unordered_set>

class BidirectionalBFS {
public:
    // 构造函数，传入图的引用，自动构建邻接表
    BidirectionalBFS(Graph& g);

    // 执行双向BFS查询，返回两个节点之间的可达性
    bool reachabilityQuery(int source, int target);

private:
    Graph& g;  // 引用图
    std::unordered_map<int, std::vector<int>> adjList;  // 邻接表，用于BFS

    // 通过BFS扩展一层
    bool bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                 std::unordered_set<int>& opposite_visited);

    // 初始化邻接表
    void buildAdjList();
};

#endif
