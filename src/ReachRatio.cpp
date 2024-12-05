// ReachRatio.cpp
#include "ReachRatio.h"
#include <vector>
#include <iostream>
#include <unordered_set>
#include <queue>

// 辅助函数：执行 BFS 并返回可达的顶点集合
std::unordered_set<int> bfs_reachable(const std::vector<std::vector<int>>& adjList, int start) {
    std::unordered_set<int> visited;
    std::queue<int> q;
    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        for (const int& neighbor : adjList[current]) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                q.push(neighbor);
            }
        }
    }

    return visited;
}


// 计算所有分区子图的可达性比例
std::unordered_map<int, float> compute_reach_ratios(const PartitionManager& pm) {
    std::unordered_map<int, float> reach_ratios;
    for (auto& partition_pair : pm.partition_subgraphs) {
        int partition_id = partition_pair.first;
        const Graph& subgraph = partition_pair.second;
        Graph& non_const_subgraph = const_cast<Graph&>(subgraph);
        float reach_ratio = compute_reach_ratio(non_const_subgraph);
        reach_ratios[partition_id] = reach_ratio;
    }
    return reach_ratios;
}

// 计算整个图中所有点对之间的可达性比例
float compute_reach_ratio(Graph& graph) {
    int n = graph.vertices.size();
    if (n == 0) {
        std::cerr << "错误：图中没有顶点。" << std::endl;
        return 0.0f;
    }

    // 创建顶点列表
    std::vector<int> vertex_list;
    vertex_list.reserve(n);
    for (int u = 0; u < n; ++u) {
        vertex_list.push_back(u);
    }

    // 构建邻接表
    std::vector<std::vector<int>> adjList(n, std::vector<int>());
    for (int i = 0; i < n; ++i) {
        int u = vertex_list[i];
        const Vertex& vertex = graph.vertices[u];
        for (const int& v : vertex.LOUT) {
            if (v >= 0 && v < n) { // 确保 v 在有效范围内
                adjList[i].push_back(v);
            }
        }
    }

    // 计算可达对数
    int reachable = 0;
    for (int i = 0; i < n; ++i) {
        std::unordered_set<int> reachable_set = bfs_reachable(adjList, i);
        reachable += reachable_set.size();
    }

    // 计算可达性比例
    // 排除自身可达的情况，使用 n*(n-1)
    float total = static_cast<float>(n * (n - 1));
    float ratio = (total > 0) ? (static_cast<float>(reachable - n) / total) : 0.0f;
    graph.set_ratio(ratio);
    return ratio;
}