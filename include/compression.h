#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "graph.h"
#include <unordered_set>

class Compression {
public:
    Compression(Graph& g);  // 压缩类的构造函数，传入图的引用

    // 删点操作
    void removeNode(int node);

    // 合并点操作
    void mergeNodes(int u, int v);

    // 删除边操作
    void removeEdge(int u, int v, bool is_directed = false);

private:
    Graph& g;  // 引用图G
};

#endif
