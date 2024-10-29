#ifndef BIDIRECTIONAL_BFS_H
#define BIDIRECTIONAL_BFS_H

#include "graph.h"
#include <vector>
#include <queue>
#include <unordered_set>

class BidirectionalBFS {
public:
    BidirectionalBFS(Graph& graph);

    bool reachabilityQuery(int source, int target);

private:
    Graph& g;
    std::vector<std::vector<int>> adjList;         // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList;  // 逆邻接表

    void buildAdjList();
    bool bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                 std::unordered_set<int>& opposite_visited, 
                 const std::vector<std::vector<int>>& adjList); // 增加邻接表参数，用于选择正向或逆向邻接表
};

#endif  // BIDIRECTIONAL_BFS_H
