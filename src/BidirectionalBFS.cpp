#include "BidirectionalBFS.h"
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>
#include <algorithm>

// 构造函数，传入图并构建邻接表和逆邻接表
BidirectionalBFS::BidirectionalBFS(Graph& graph) : g(graph) {
    buildAdjList();  // 构建邻接表和逆邻接表
}

// 初始化邻接表和逆邻接表
void BidirectionalBFS::buildAdjList() {
    adjList.resize(g.vertices.size());
    reverseAdjList.resize(g.vertices.size());  // 初始化逆邻接表

    for (int node = 0; node < g.vertices.size(); ++node) {
        const auto& neighbors = g.vertices[node].LOUT;  // 使用LOUT构建邻接表
        for (int neighbor : neighbors) {
            adjList[node].push_back(neighbor);          // 邻接表
            reverseAdjList[neighbor].push_back(node);   // 逆邻接表，反向连接
        }
    }
}


void BidirectionalBFS::offline_industry()
{
    int a;
    return;
}

// 双向BFS查询
bool BidirectionalBFS::reachability_query(int source, int target) {
    if (source == target) return true;
    if (source >= adjList.size() || target >= adjList.size() || source < 0 || target < 0) {
        return false; // 如果超出范围，直接返回不可达
    }
        // 检查是否为孤立节点（无出边且无入边）
    if (adjList[source].empty() && reverseAdjList[source].empty()) return false;
    if (adjList[target].empty() && reverseAdjList[target].empty()) return false;


    
    // BFS队列
    std::queue<int> queueFromSource, queueFromTarget;
    std::unordered_set<int> visitedFromSource, visitedFromTarget;

    // 初始化
    queueFromSource.push(source);
    visitedFromSource.insert(source);

    queueFromTarget.push(target);
    visitedFromTarget.insert(target);

    // 开始双向BFS
    while (!queueFromSource.empty() && !queueFromTarget.empty()) {
        // 从source侧扩展一步
        if (bfsStep(queueFromSource, visitedFromSource, visitedFromTarget, adjList)) {
            return true;
        }

        // 从target侧扩展一步，使用逆邻接表
        if (bfsStep(queueFromTarget, visitedFromTarget, visitedFromSource, reverseAdjList)) {
            return true;
        }
    }

    return false;
}

// 单次BFS步进，扩展一层
bool BidirectionalBFS::bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                               std::unordered_set<int>& opposite_visited, 
                               const std::vector<std::vector<int>>& adjList) {
    if (queue.empty()) return false;

    int current = queue.front();
    queue.pop();

    // 检查邻接表有效性
    if (current >= adjList.size()) return false;

    // 获取邻居节点
    const auto& neighbors = adjList[current];
    for (int neighbor : neighbors) {
        // 如果在对方的访问集合中，说明路径相遇
        if (opposite_visited.find(neighbor) != opposite_visited.end()) {
            return true;
        }

        // 如果未访问过该节点，则加入访问队列
        if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            queue.push(neighbor);
        }
    }

    return false;
}
