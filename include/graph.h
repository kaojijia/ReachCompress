#ifndef GRAPH_H
#define GRAPH_H
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

// 表示节点的结构
struct Vertex {
    int label;
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

    bool hasEdge(int u, int v);
    // 删除边
    void removeEdge(int u, int v, bool is_directed = true);

    // 删除点
    void removeNode(int node);

    // 打印边信息
    void printEdges();

    // 边和点的数量存进去
    std::pair<int, int> statics(const std::string &filename = "") const;

private:
    bool store_edges;  // 控制是否存储边集
};

#endif  // GRAPH_H