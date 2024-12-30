#ifndef GRAPH_H
#define GRAPH_H
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/resource.h>


// 表示节点的结构
struct Vertex {
    int partition_id = -1;  // 分区ID
    std::vector<int> LOUT;  // 出度
    std::vector<int> LIN;   // 入度
    int in_degree = 0;
    int out_degree = 0;
    uint32_t equivalance = 999999999; //等价类ID（强连通分量压缩用）
};

// 图的结构，支持可选的邻接表逆邻接表存储
class Graph {
public:
    std::vector<Vertex> vertices;  //点集
    std::vector<std::vector<int>> adjList;  //邻接表
    std::vector<std::vector<int>> reverseAdjList; //逆邻接表
    // 构造函数，控制是否存储边集
    Graph(bool store_edges = true);
    void set_max_node_id(uint32_t max_node_id){
        this->max_node_id = max_node_id;
        this->vertices.resize(max_node_id + 1);
    };

    void addEdge_simple(int u, int v, bool is_directed);

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
    
    
    long get_partition_degree(int target_patition) const;

    int get_partition_id(int node) const;
    bool set_partition_id(int node, int part_id);
    void setFilename(const std::string &name) {
        filename = name;
    }
    std::string getFilename() const {
        return filename;
    }
    void set_ratio(float r) {
        ratio = r;
    }
    float get_ratio() const {
        return ratio;
    }

    size_t get_num_vertices() const{
        return num_vertices;
    }

    size_t get_num_edges() const{
        return num_edges;
    }
private:
    uint32_t max_node_id = 0;
    bool store_edges;  // 控制是否存储边集
    std::string filename;
    float ratio;
    size_t num_vertices;
    size_t num_edges;
    
};

#endif  // GRAPH_H