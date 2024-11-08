#include "graph.h"
#include <iostream>
#include <algorithm>  // 确保包含算法库
#include <fstream>

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

bool Graph::hasEdge(int u, int v)
{
    if (u >= vertices.size() || v >= vertices.size()) {
        return false;
    }
    if (vertices[u].out_degree == 0 || vertices[v].in_degree == 0) {
        return false;
    }
    if (u < vertices.size()) {
        auto& out_edges = vertices[u].LOUT;
        return std::find(out_edges.begin(), out_edges.end(), v) != out_edges.end();
    }
    return false;
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

// 删除节点
void Graph::removeNode(int node) {
    if (node >= vertices.size()) return;

    // 删除与该节点相关的所有出边和入边
    for (int v : vertices[node].LOUT) {
        removeEdge(node, v);
    }
    for (int u : vertices[node].LIN) {
        removeEdge(u, node);
    }

    // 清空该节点的出边和入边列表
    vertices[node].LOUT.clear();
    vertices[node].LIN.clear();
    vertices[node].out_degree = 0;
    vertices[node].in_degree = 0;
}

// 打印边信息
void Graph::printEdges() {
    std::cout << "Edges in the graph:" << std::endl;
    for (const auto& edge : edges) {
        std::cout << edge.first << " -> " << edge.second << std::endl;
    }
}

std::pair<int, int> Graph::statics(const std::string& filename) const {
    int node_count = 0;
    int edge_count = 0;

    for (const auto& vertex : vertices) {
        if (!vertex.LOUT.empty() || !vertex.LIN.empty()) {
            node_count++;
            edge_count += vertex.LOUT.size();
        }
    }

    if (filename.empty()) {
        return std::make_pair(node_count, edge_count);
    }

    std::ofstream outfile(PROJECT_ROOT_DIR + filename, std::ios::out | std::ios::trunc);
    if (outfile.is_open()) {
        outfile << "Node count: " << node_count << std::endl;
        outfile << "Edge count: " << edge_count << std::endl;
        outfile.close();
    } else {
        std::cerr << "Failed to open file: " << PROJECT_ROOT_DIR + filename << std::endl;
    }

    return std::make_pair(node_count, edge_count);
}
