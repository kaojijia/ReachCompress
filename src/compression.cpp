#include "compression.h"
#include <algorithm>  // 确保包含算法库

// 构造函数，接受图的引用
Compression::Compression(Graph& graph) : g(graph) {}

// 删除节点
void Compression::removeNode(int node) {
    for (int neighbor : g.vertices[node].LOUT) {
        auto& lin_neighbors = g.vertices[neighbor].LIN;
        lin_neighbors.erase(std::remove(lin_neighbors.begin(), lin_neighbors.end(), node), lin_neighbors.end());
        g.vertices[neighbor].in_degree--;
    }
    for (int neighbor : g.vertices[node].LIN) {
        auto& lout_neighbors = g.vertices[neighbor].LOUT;
        lout_neighbors.erase(std::remove(lout_neighbors.begin(), lout_neighbors.end(), node), lout_neighbors.end());
        g.vertices[neighbor].out_degree--;
    }
    g.vertices[node].LOUT.clear();
    g.vertices[node].LIN.clear();
}

// 合并节点
void Compression::mergeNodes(int u, int v) {
    for (int neighbor : g.vertices[v].LOUT) {
        if (std::find(g.vertices[u].LOUT.begin(), g.vertices[u].LOUT.end(), neighbor) == g.vertices[u].LOUT.end()) {
            g.vertices[u].LOUT.push_back(neighbor);
        }
        if (std::find(g.vertices[neighbor].LIN.begin(), g.vertices[neighbor].LIN.end(), u) == g.vertices[neighbor].LIN.end()) {
            g.vertices[neighbor].LIN.push_back(u);
        }
    }
    for (int neighbor : g.vertices[v].LIN) {
        if (std::find(g.vertices[u].LIN.begin(), g.vertices[u].LIN.end(), neighbor) == g.vertices[u].LIN.end()) {
            g.vertices[u].LIN.push_back(neighbor);
        }
        if (std::find(g.vertices[neighbor].LOUT.begin(), g.vertices[neighbor].LOUT.end(), u) == g.vertices[neighbor].LOUT.end()) {
            g.vertices[neighbor].LOUT.push_back(u);
        }
    }

    // 删除节点v
    removeNode(v);
}

// 删除边
void Compression::removeEdge(int u, int v, bool is_directed) {
    g.removeEdge(u, v, is_directed);
}