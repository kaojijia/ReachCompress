#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

//TODO: 重构NodeMapping结构体，里面的in和out映射应该有个迭代关系



struct NodeMapping {
    std::vector<int> inMapping;   // 入映射
    std::vector<int> outMapping;  // 出映射
    std::vector<int> sccMapping;  // 强连通分量合并映射
};

class Compression {
public:
    Compression(const Graph& graph);  // 构造函数，接受图的引用

    //get方法
    const Graph &getGraph() const;
    const std::unordered_map<int, NodeMapping> &getMapping() const;

    int findMapping(int node) const; // 查找被合并成哪个节点

    void removeNode(int node);  // 删除节点
    void removeEdge(int u, int v); // 删除边
    
    void compressRAG();  // 压缩有向无环子图
    
    void compress(int u, int v);  // 核心压缩方法

    void mergeIn1Out1Nodes();  // 合并入度为1出度为1的节点
    void mergeRouteNodes(); //合并中继点

    void classifyVertex(int vertex); // 归类节点
    void del_nodes();
    void mergeNodes(int u, int v, int w);    // 合并节点，需要保证顺序u->v->w
    void mergeNodes(int u, int v, bool is_reversed, bool is_delete); // 单向合并节点，标注方向（默认u->v）和是否删除

    void removeRedundantEdges(int u, int w);  // 删除多余的边
private:
    Graph g;  //存储图的副本

    std::unordered_set<int> in1outN;  // 入度为1，出度大于1的节点
    std::unordered_set<int> inNout1;  // 出度为1，入度大于1的节点
    std::unordered_set<int> in1out1;  // 入度为1，出度为1的节点
    std::unordered_set<int> degree2;  // 某一个度为2的节点


    const std::unordered_map<int, std::vector<int>> &getSccMapping() const;
    const std::unordered_map<int, std::vector<int>> &getOutMapping() const;
    const std::unordered_map<int, std::vector<int>> &getInMapping() const;

    std::unordered_map<int, std::vector<int>> sccMapping;  // 强连通分量合并映射关系
    std::unordered_map<int, std::vector<int>> outMapping;  // 原先由这个点指向的点
    std::unordered_map<int, std::vector<int>> inMapping;   // 反过来代表的点
    
    std::unordered_map<int, NodeMapping> mapping;  // 节点映射关系
};

#endif


