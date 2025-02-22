#include "graph.h"
#include <iostream>
#include <algorithm>  // 确保包含算法库
#include <fstream>
#include <sstream>
using namespace std;

//输出当前时间
std::string getCurrentTimestampofGraph() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
    return ss.str();
}

// 构造函数，指定是否存储边集
Graph::Graph(bool store_edges) : store_edges(store_edges) {
    this->num_edges=0;
    this->num_vertices=0;
}





long Graph::get_partition_degree(int target_patition) const
{
    long total_degree = 0;
    for(auto const i:vertices){
        if(i.partition_id == target_patition){
            total_degree += i.out_degree;
        }
    }
    return 0;
}


int Graph::get_partition_id(int node) const
{
    return vertices[node].partition_id;
}

bool Graph::set_partition_id(int node, int part_id)
{
    if (node >= vertices.size()) return false;
    // if (vertices[node].in_degree==0&&vertices[node].out_degree==0)return false;
    vertices[node].partition_id = part_id;
    return true;
}

// 简化版本
void Graph::addEdge_simple(int u, int v, bool is_directed) {
    // if (u==v) return;
    //跳过已有边
    if (u < vertices.size()) {
        for(auto i:vertices[u].LOUT){
            if(i==v)return;
        }
    }
    vertices[u].LOUT.push_back(v);
    vertices[v].LIN.push_back(u);
    vertices[u].out_degree++;
    vertices[v].in_degree++;

    this->num_edges++;
    if(vertices[u].out_degree==1&&vertices[u].in_degree==0)
        this->num_vertices++;
    if(vertices[v].out_degree==0&&vertices[v].in_degree==1)
        this->num_vertices++;

}
// 添加边到图
void Graph::addEdge(int u, int v, bool is_directed) {
    if (u == v) return;

    // 判断是否需要 resize
    if (u >= vertices.size()) 
        vertices.resize(u + 1);
    if (v >= vertices.size()) 
        vertices.resize(v + 1);

    auto& out_edges = vertices[u].LOUT;
    auto& in_edges = vertices[v].LIN;

    // 插入边并保持有序
    auto out_pos = std::lower_bound(out_edges.begin(), out_edges.end(), v);
    if (out_pos == out_edges.end() || *out_pos != v) {
        out_edges.insert(out_pos, v);
        vertices[u].out_degree++;

        auto in_pos = std::lower_bound(in_edges.begin(), in_edges.end(), u);
        in_edges.insert(in_pos, u);
        vertices[v].in_degree++;

        // 更新顶点和边的数量
        this->num_edges++;
        if (vertices[u].out_degree == 1 && vertices[u].in_degree == 0)
            this->num_vertices++;
        if (vertices[v].out_degree == 0 && vertices[v].in_degree == 1)
            this->num_vertices++;

        // 如果需要存储边集，确保 adjList 和 reverseAdjList 也被正确调整
        if (store_edges) {
            if (u >= adjList.size()) adjList.resize(u + 1);
            if (v >= reverseAdjList.size()) reverseAdjList.resize(v + 1);

            auto adj_pos = std::lower_bound(adjList[u].begin(), adjList[u].end(), v);
            adjList[u].insert(adj_pos, v);

            auto rev_adj_pos = std::lower_bound(reverseAdjList[v].begin(), reverseAdjList[v].end(), u);
            reverseAdjList[v].insert(rev_adj_pos, u);
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
    if (u >= vertices.size() || v >= vertices.size()) return;
    auto remove_edge = [](std::vector<int>& vec, int val) {
        auto it = std::lower_bound(vec.begin(), vec.end(), val);
        if (it != vec.end() && *it == val) {
            vec.erase(it);
        }
    };

    remove_edge(vertices[u].LOUT, v);
    remove_edge(vertices[v].LIN, u);
    vertices[u].out_degree--;
    vertices[v].in_degree--;

    // 更新边的数量
    this->num_edges--;
    // 更新点的数量
    if(vertices[u].out_degree==0&&vertices[u].in_degree==0){
        this->num_vertices--;
        vertices[u].partition_id = 999999990;
    }

    if(vertices[v].out_degree==0&&vertices[v].in_degree==0){
        this->num_vertices--;
        vertices[v].partition_id = 999999990;
    }

    if (store_edges) {
        remove_edge(adjList[u], v);
        remove_edge(reverseAdjList[v], u);
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
    
    // 清空邻接表和逆邻接表中的信息
    if (store_edges && node < adjList.size()) {
        adjList[node].clear();
        reverseAdjList[node].clear();
    }
}

// 打印边信息
void Graph::printEdges() {
    if (!store_edges) {
        std::cout << "Adjacency lists are not stored." << std::endl;
        return;
    }
    std::cout << "Edges in the graph:" << std::endl;
    for (size_t u = 0; u < adjList.size(); ++u) {
        for (int v : adjList[u]) {
            std::cout << u << " -> " << v << std::endl;
        }
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


// DFS 辅助函数
bool Graph::isCyclicUtil(int v, std::vector<bool> &visited, std::vector<bool> &recursionStack) {
    if (!visited[v]) {
        // 标记当前节点为已访问，并加入递归栈
        visited[v] = true;
        recursionStack[v] = true;

        // 遍历当前节点的所有邻接点
        for (int neighbor : vertices[v].LOUT) {
            if (!visited[neighbor] && isCyclicUtil(neighbor, visited, recursionStack))
                return true;
            else if (recursionStack[neighbor])
                return true;
        }
    }

    // 将节点从递归栈中移除
    recursionStack[v] = false;
    return false;
}

// 检测是否有环
bool Graph::hasCycle() {
    int num_vertices = vertices.size();
    std::vector<bool> visited(num_vertices, false);
    std::vector<bool> recursionStack(num_vertices, false);

    // 对图中的每个节点调用 DFS
    for (int i = 0; i < num_vertices; i++) {
        if (!visited[i] && isCyclicUtil(i, visited, recursionStack))
            return true;
    }

    return false;
}

bool Graph::isExist(int v)
{
    if(v>=vertices.size())
        return false;
    if(vertices[v].in_degree==0&&vertices[v].out_degree==0)
        return false;
    return true;
}
