#include "graph.h"
#include "BidirectionalBFS.h"
#include <vector>
#include <algorithm>
#include <utility>

// 函数实现
std::vector<std::pair<int, int>> add_edges_by_degree_threshold(Graph& graph, int degree_threshold) {
    // 存储新增的边
    std::vector<std::pair<int, int>> addedEdges;

    // 1. 筛选出度大于阈值的顶点
    std::vector<int> highDegreeVertices;
    for (int i = 0; i < graph.vertices.size(); ++i) {
        int degree = graph.vertices[i].LOUT.size() + graph.vertices[i].LIN.size();
        if (degree > degree_threshold) {
            highDegreeVertices.push_back(i);
        }
    }

    // 2. 在高阶顶点之间检查可达性并加边
    BidirectionalBFS bibfs(graph);
    for (size_t i = 0; i < highDegreeVertices.size(); ++i) {
        for (size_t j = i + 1; j < highDegreeVertices.size(); ++j) {
            int u = highDegreeVertices[i];
            int v = highDegreeVertices[j];

            // 检查是否可达，且边不存在
            if (bibfs.reachability_query(u, v) && !graph.hasEdge(u, v)) {
                graph.addEdge(u, v); // 添加边
                addedEdges.emplace_back(u, v); // 记录新增边
            }
        }
    }

    return addedEdges;
}
