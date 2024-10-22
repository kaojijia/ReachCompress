#include "compression.h"

// 构造函数，接受图的引用
Compression::Compression(Graph& graph) : g(graph) {}

// 删除节点
void Compression::removeNode(int node) {
    for (int neighbor : g.vertices[node].LOUT) {
        g.vertices[neighbor].LIN.erase(node);
        g.vertices[neighbor].in_degree--;
    }
    for (int neighbor : g.vertices[node].LIN) {
        g.vertices[neighbor].LOUT.erase(node);
        g.vertices[neighbor].out_degree--;
    }
    g.vertices.erase(node);  // 从图中删除节点
}

// 合并节点
void Compression::mergeNodes(int u, int v) {
    for (int neighbor : g.vertices[v].LOUT) {
        g.vertices[u].LOUT.insert(neighbor);
        g.vertices[neighbor].LIN.insert(u);
    }
    for (int neighbor : g.vertices[v].LIN) {
        g.vertices[u].LIN.insert(neighbor);
        g.vertices[neighbor].LOUT.insert(u);
    }

    // 删除节点v
    removeNode(v);
}

// 删除边
void Compression::removeEdge(int u, int v, bool is_directed) {
    g.removeEdge(u, v, is_directed);
}
