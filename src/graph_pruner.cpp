#include "graph_pruner.h"

GraphPruner::GraphPruner(Graph& graph) : g(graph) {}

// 裁剪孤立节点
void GraphPruner::pruneIsolatedNodes() {
    std::vector<int> to_remove;
    for (const auto& pair : g.vertices) {
        int node = pair.first;
        const auto& vertex = pair.second;
        if (vertex.LOUT.empty() && vertex.LIN.empty()) {
            to_remove.push_back(node);  // 标记要移除的孤立节点
        }
    }

    // 移除标记的孤立节点
    for (int node : to_remove) {
        g.vertices.erase(node);
    }
}

// 根据度数裁剪
void GraphPruner::pruneByDegree(int min_in_degree, int min_out_degree) {
    std::vector<int> to_remove;
    for (const auto& pair : g.vertices) {
        int node = pair.first;
        const auto& vertex = pair.second;
        if (vertex.in_degree < min_in_degree || vertex.out_degree < min_out_degree) {
            to_remove.push_back(node);  // 标记要移除的节点
        }
    }

    // 移除标记的节点
    for (int node : to_remove) {
        g.vertices.erase(node);
    }
}

// 根据指定的节点集合裁剪
void GraphPruner::pruneByNodeSet(const std::unordered_set<int>& node_set) {
    for (int node : node_set) {
        g.vertices.erase(node);  // 移除在集合中的节点
    }
}
