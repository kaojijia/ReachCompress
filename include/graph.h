#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_set>
#include <unordered_map>
#include <vector>

// 表示边的结构
struct Edge {
    int from;
    int to;
    Edge(int u, int v) : from(u), to(v) {}
};

// 表示节点的结构
struct Vertex {
    std::unordered_set<int> LOUT;  // 出度
    std::unordered_set<int> LIN;   // 入度
    int in_degree = 0;
    int out_degree = 0;
};

// 图的结构，支持可选的边集存储
class Graph {
public:
    std::unordered_map<int, Vertex> vertices;  // 邻接表存储节点
    std::vector<Edge> edges;  // 边集存储（用于显式存储所有边）

    // 构造函数，控制是否存储边集
    Graph(bool store_edges = true);

    // 添加边
    void addEdge(int u, int v, bool is_directed = false);

    // 删除边
    void removeEdge(int u, int v, bool is_directed = false);

    // 打印边信息
    void printEdges();

private:
    bool store_edges;  // 控制是否存储边集
};

#endif
