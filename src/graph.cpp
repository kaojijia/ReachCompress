#include "graph.h"
#include <iostream>
#include <algorithm>  // 确保包含算法库

// 构造函数，指定是否存储边集
Graph::Graph(bool store_edges) : store_edges(store_edges) {


}

// 添加边到图
void Graph::addEdge(int u, int v, bool is_directed) {
    if (u >= vertices.size()) vertices.resize(u + 1);
    if (v >= vertices.size()) vertices.resize(v + 1);

    vertices[u].LOUT.push_back(v);
    vertices[v].LIN.push_back(u);
    vertices[u].out_degree++;
    vertices[v].in_degree++;

    if (store_edges) {
        edges.emplace_back(u, v);  // 将边添加到边集中
        if (!is_directed) {
            edges.emplace_back(v, u);  // 无向图添加反向边
        }
    }
}

// 删除边
void Graph::removeEdge(int u, int v, bool is_directed) {
    auto remove_edge = [](std::vector<int>& vec, int val) {
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
    };

    remove_edge(vertices[u].LOUT, v);
    remove_edge(vertices[v].LIN, u);
    vertices[u].out_degree--;
    vertices[v].in_degree--;

    if (store_edges) {
        // 使用 std::remove_if 删除边
        edges.erase(std::remove_if(edges.begin(), edges.end(),
                                   [u, v](const std::pair<int, int>& e) { return e.first == u && e.second == v; }),
                    edges.end());

        if (!is_directed) {
            edges.erase(std::remove_if(edges.begin(), edges.end(),
                                       [u, v](const std::pair<int, int>& e) { return e.first == v && e.second == u; }),
                        edges.end());
        }
    }
}

// 打印边信息
void Graph::printEdges() {
    std::cout << "Edges in the graph:" << std::endl;
    for (const auto& edge : edges) {
        std::cout << edge.first << " -> " << edge.second << std::endl;
    }
}