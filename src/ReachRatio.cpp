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

// 计算指定分区内所有点对之间的可达性比例
float compute_reach_ratio(const PartitionManager& pm, int target_partition) {
    // 获取目标分区内的所有顶点
    const std::unordered_set<int>& vertices_in_partition = pm.get_vertices_in_partition(target_partition);
    if (vertices_in_partition.empty()) {
        std::cerr << "错误：分区 " << target_partition << " 内没有顶点。" << std::endl;
        return 0.0f;
    }

    int n = vertices_in_partition.size();

    // 创建顶点列表
    std::vector<int> vertex_list;
    vertex_list.reserve(n);
    for (const int& v : vertices_in_partition) {
        vertex_list.push_back(v);
    }

    // 构建邻接表（仅包含分区内的顶点）
    std::vector<std::vector<int>> adjList(n, std::vector<int>());
    const Graph& graph = pm.get_graph();

    for (int i = 0; i < n; ++i) {
        int u = vertex_list[i];
        if (u < 0 || u >= static_cast<int>(graph.vertices.size())) {
            std::cerr << "错误：顶点 " << u << " 不存在于图中。" << std::endl;
            continue;
        }
        const Vertex& vertex = graph.vertices[u];
        for (const int& v : vertex.LOUT) {
            // 仅考虑分区内的顶点
            if (vertices_in_partition.find(v) != vertices_in_partition.end()) {
                // 找到顶点 v 在分区内的索引
                // 因为 vertex_list 是按顺序存储的，所以可以通过遍历查找
                // 为了提高效率，可以预先构建一个映射
                // 这里为了简化，使用线性搜索
                auto it = std::find(vertex_list.begin(), vertex_list.end(), v);
                if (it != vertex_list.end()) {
                    int j = std::distance(vertex_list.begin(), it);
                    adjList[i].push_back(j);
                }
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
    float reach_ratio = (total > 0) ? (static_cast<float>(reachable - n) / total) : 0.0f;

    return reach_ratio;
}

// 计算整个图中所有点对之间的可达性比例
float compute_reach_ratio(const Graph& graph) {
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
    float reach_ratio = (total > 0) ? (static_cast<float>(reachable - n) / total) : 0.0f;

    return reach_ratio;
}