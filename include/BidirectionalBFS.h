#ifndef BIDIRECTIONAL_BFS_H
#define BIDIRECTIONAL_BFS_H

#include "graph.h"
#include "CSR.h"
#include "Algorithm.h"
#include <vector>
#include <queue>
#include <unordered_set>

class BidirectionalBFS : public Algorithm {
public:
    BidirectionalBFS(Graph& graph);

    // bool reachability(int source, int target) override;

    void offline_industry() override;

    bool reachability_query(int source, int target) override;

    // 找路径是否可达，如果可达返回路径，否则返回空。第三个参数，分区内搜索的时候要设置成true
    std::vector<int> findPath(int source, int target, int partition_number = -1); 
    std::unordered_map<std::string, size_t> getIndexSizes() const override {
        return {};
    }

private:
    Graph& g;
    CSRGraph csr;
    std::vector<std::vector<int>> adjList;         // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList;  // 逆邻接表

    void buildAdjList();
    bool bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                 std::unordered_set<int>& opposite_visited, 
                 const std::vector<std::vector<int>>& adjList); // 增加邻接表参数，用于选择正向或逆向邻接表

};

#endif  // BIDIRECTIONAL_BFS_H
