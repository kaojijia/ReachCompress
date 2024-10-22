#include "graph.h"
#include <iostream>
#include <algorithm>  // 确保包含算法库

// 构造函数，指定是否存储边集
Graph::Graph(bool store_edges) : store_edges(store_edges) {}

// 添加边到图
void Graph::addEdge(int u, int v, bool is_directed) {
    vertices[u].LOUT.insert(v);
    vertices[v].LIN.insert(u);
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
    if (vertices[u].LOUT.erase(v) > 0) {
        vertices[u].out_degree--;
    }
    if (vertices[v].LIN.erase(u) > 0) {
        vertices[v].in_degree--;
    }

    if (store_edges) {
        // 使用 std::remove_if 删除边
        edges.erase(std::remove_if(edges.begin(), edges.end(),
                                   [u, v](const Edge& e) { return e.from == u && e.to == v; }),
                    edges.end());

        if (!is_directed) {
            edges.erase(std::remove_if(edges.begin(), edges.end(),
                                       [u, v](const Edge& e) { return e.from == v && e.to == u; }),
                        edges.end());
        }
    }
}

// 打印边信息
void Graph::printEdges() {
    std::cout << "Edges in the graph:" << std::endl;
    for (const Edge& edge : edges) {
        std::cout << edge.from << " -> " << edge.to << std::endl;
    }
}
