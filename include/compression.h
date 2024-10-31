#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Compression {
public:
    Compression(Graph& graph);  // 构造函数，接受图的引用

    void removeNode(int node);  // 删除节点
    void mergeNodes(int u, int v);  // 合并节点
    bool isMerged(int node) const;  // 检查节点是否已合并
    void markAsMerged(int node);  // 标记节点已合并
    void removeEdge(int u, int v, bool is_directed = true); // 删除边
    void removeEdgeFromNode(int u, int v);  // 辅助函数：从节点 u 中删除边 v
    void compress(int u, int v);  // 核心压缩方法
    void compressRAG();  // 压缩有向无环子图
    void mergeIn1Out1Nodes();  // 合并入度为1出度为1的节点
    void classifyVertex(int vertex);  // 归类节点
    void mergeIntermediateNodes();  // 合并中间节点

private:
    Graph& g;  // 引用图
    std::unordered_map<int, std::vector<int>> mapping;  // 节点映射关系
    std::unordered_set<int> in1outN;  // 入度为1，出度大于1的节点
    std::unordered_set<int> inNout1;  // 出度为1，入度大于1的节点
    std::unordered_set<int> in1out1;  // 入度为1，出度为1的节点
    std::unordered_set<int> mergedNodes;  // 已合并的节点集合

    std::vector<int> topologicalSort();  // 拓扑排序
    void dfs(int node, std::vector<bool>& visited, std::vector<int>& result);  // 深度优先搜索
};

#endif
