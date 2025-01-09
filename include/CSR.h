#ifndef CSR_H
#define CSR_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring> // for memcpy

#include "graph.h" 


class CSRGraph {
public:
    // 出边和入边的 CSR 结构
    uint32_t* out_column_indices;  // 存储出边的列索引
    uint32_t* out_row_pointers;    // 存储出边的每一行的起始位置索引,从0开始，最后一位是总边数，倒数第二位的index是max_node_id
    uint32_t* in_column_indices;   // 存储入边的列索引
    uint32_t* in_row_pointers;     // 存储入边的每一行的起始位置索引

    // 节点的分区信息
    int16_t* partitions;           // 存储每个节点的分区号，预设值为 -1

    uint32_t max_node_id;          // 最大节点id=数组长度-2
    uint32_t num_edges;            // 边数量
    uint32_t num_nodes;            // 节点数量

    // 构造函数
    CSRGraph()
        : out_column_indices(nullptr), out_row_pointers(nullptr),
          in_column_indices(nullptr), in_row_pointers(nullptr),
          partitions(nullptr), max_node_id(0), num_edges(0), num_nodes(0) {}

    // 析构函数
    ~CSRGraph() {
        delete[] out_column_indices;
        delete[] out_row_pointers;
        delete[] in_column_indices;
        delete[] in_row_pointers;
        delete[] partitions;
    }

    // 创建空的CSR图
    bool createEmptyCSR(uint32_t num_vertices);

    // 从边集文件中读取图数据并构建 CSR 结构
    bool fromFile(const std::string& filename);

    //  从现有的图创建CSR结构
    bool fromGraph(const Graph& graph);

    // 获取某个节点的所有出边
    uint32_t* getOutgoingEdges(uint32_t node, uint32_t& degree) const;

    // 获取某个节点的所有入边
    uint32_t* getIncomingEdges(uint32_t node, uint32_t& degree) const;

    // 获取某个节点的出度
    uint32_t getOutDegree(uint32_t node) const;

    // 获取某个节点的入度
    uint32_t getInDegree(uint32_t node) const;

    // 增加节点
    bool addNode();

    // 删除节点
    bool removeNode(uint32_t node);

    // 增加边
    bool addEdge(uint32_t u, uint32_t v);

    // 删除边
    bool removeEdge(uint32_t u, uint32_t v);

    // 修改节点的分区号
    bool setPartition(uint32_t node, int16_t partition);

    // 获取节点的分区号
    int getPartition(uint32_t node) const;

    // 打印图的基本信息
    void printInfo() const;
    
    // 打印图的所有信息
    void printAllInfo() const;

    // 打印CSR结构
    void printCSRs() const;

    // 返回CSR结构所占内存
    uint64_t getMemoryUsage() const;

    // 检查节点是否存在
    bool nodeExist(uint32_t node) const;
    
    // 统计顶点个数
    uint32_t getNodesNum() const;
    // 设置节点个数
    uint32_t setNodesNum(uint32_t);

    // 重设节点个数，之前创建空的CSR的时候会设置一个很大的数，
    // 现在要将节点个数设置为实际大小，减少空间占用
    void resetNodesNum(){};

private:

};

#endif // CSR_H