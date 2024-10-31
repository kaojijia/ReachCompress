#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <algorithm>

// 表示节点的结构
struct Vertex {
    std::vector<int> LOUT;  // 出度
    std::vector<int> LIN;   // 入度
    int in_degree = 0;
    int out_degree = 0;
};

// 图的结构，支持可选的邻接表逆邻接表存储
class Graph {
public:
    std::vector<Vertex> vertices;  
    std::vector<std::pair<int, int>> edges;  // 边集存储（用于显式存储所有边）
    // 构造函数，控制是否存储边集
    Graph(bool store_edges = false);

    // 添加边
    void addEdge(int u, int v, bool is_directed = true);

    // 删除边
    void removeEdge(int u, int v, bool is_directed = true);

    // 打印边信息
    void printEdges();

private:
    bool store_edges;  // 控制是否存储边集

    std::vector<std::vector<int>> adjList;         // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList;  // 逆邻接表

    void buildAdjList();

};

#endif  // GRAPH_H