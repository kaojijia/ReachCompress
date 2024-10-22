#include "BidirectionalBFS.h"
#include <iostream>

// 构造函数，传入图并构建邻接表
BidirectionalBFS::BidirectionalBFS(Graph& graph) : g(graph) {
    buildAdjList();  // 构建邻接表
}

// 初始化邻接表，将图的顶点和边构建为邻接表结构
void BidirectionalBFS::buildAdjList() {
    for (const auto& pair : g.vertices) {
        int node = pair.first;
        const auto& neighbors = pair.second.LOUT;  // 使用LOUT来构建邻接表
        for (int neighbor : neighbors) {
            adjList[node].push_back(neighbor);  // 为该节点添加所有邻居
        }
    }
}

// 双向BFS查询
bool BidirectionalBFS::reachabilityQuery(int source, int target) {
    // 如果源点和目标点相同，直接返回可达
    if (source == target) return true;

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
        if (bfsStep(queueFromSource, visitedFromSource, visitedFromTarget)) {
            return true;
        }

        // 从target侧扩展一步
        if (bfsStep(queueFromTarget, visitedFromTarget, visitedFromSource)) {
            return true;
        }
    }

    // 如果队列为空且没有相遇，则不可达
    return false;
}

// 单次BFS步进，扩展一层
bool BidirectionalBFS::bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                               std::unordered_set<int>& opposite_visited) {
    int current = queue.front();
    queue.pop();

    // 获取邻居节点
    const auto& neighbors = adjList[current];

    // 遍历当前节点的所有邻居
    for (int neighbor : neighbors) {
        // 如果已经在对方的访问集合中，说明路径相遇，返回可达
        if (opposite_visited.find(neighbor) != opposite_visited.end()) {
            return true;
        }

        // 如果还没有访问过该节点，则将其加入访问队列
        if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            queue.push(neighbor);
        }
    }

    return false;
}
