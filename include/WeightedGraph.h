//// filepath: /root/Projects/ReachCompress/include/WeightedGraph.h
#ifndef WEIGHTED_GRAPH_H
#define WEIGHTED_GRAPH_H

#include <vector>
#include <limits>
#include <stdexcept>
#include <memory>
#include <queue>
#include <utility>


// 嵌入式并查集实现，专为WeightedGraph设计
class GraphDisjointSets {
public:
    GraphDisjointSets(size_t n) : parent(n), rank(n, 0) {
        for (size_t i = 0; i < n; i++)
            parent[i] = i;
    }
    
    int find(int x) {
        if (parent[x] != x)
            parent[x] = find(parent[x]); // 路径压缩
        return parent[x];
    }
    
    void merge(int x, int y) {
        int root_x = find(x);
        int root_y = find(y);
        
        if (root_x == root_y)
            return;
            
        // 按秩合并
        if (rank[root_x] < rank[root_y])
            parent[root_x] = root_y;
        else if (rank[root_x] > rank[root_y])
            parent[root_y] = root_x;
        else {
            parent[root_y] = root_x;
            rank[root_x]++;
        }
    }
    
private:
    std::vector<int> parent;
    std::vector<int> rank;
};


// 无向带权图类 - 简化版本
class WeightedGraph {
public:
    // 构造函数
    WeightedGraph(size_t num_vertices = 0, size_t estimated_edges_per_vertex = 10) : 
        adj_list(num_vertices) {
        // 预分配空间以减少重新分配
        for (auto& neighbors : adj_list) {
            neighbors.reserve(estimated_edges_per_vertex);
        }
    }
    
    // 批量添加顶点
    void addVertices(size_t count, size_t estimated_edges_per_vertex = 10) {
        size_t original_size = adj_list.size();
        adj_list.resize(original_size + count);
        
        for (size_t i = original_size; i < adj_list.size(); i++) {
            adj_list[i].reserve(estimated_edges_per_vertex);
        }
    }
    
    // 添加顶点，返回顶点ID
    int addVertex(size_t estimated_edges = 10) {
        adj_list.push_back(std::vector<std::pair<int, int>>());
        adj_list.back().reserve(estimated_edges);
        return adj_list.size() - 1;
    }
    
    // 批量添加边
    void addEdges(const std::vector<std::tuple<int, int, int>>& edges) {
        for (const auto& [u, v, weight] : edges) {
            if (u < 0 || u >= static_cast<int>(adj_list.size()) ||
                v < 0 || v >= static_cast<int>(adj_list.size()))
                throw std::invalid_argument("Vertex does not exist");
                
            adj_list[u].emplace_back(v, weight);
            adj_list[v].emplace_back(u, weight); // 无向图需要双向连接
        }
    }
    
    // 添加边及其权重
    void addEdge(int u, int v, int weight) {
        if (u < 0 || u >= static_cast<int>(adj_list.size()) ||
            v < 0 || v >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");
            
        adj_list[u].emplace_back(v, weight);
        adj_list[v].emplace_back(u, weight); // 无向图需要双向连接
    }
    
    // 获取顶点数
    size_t numVertices() const {
        return adj_list.size();
    }
    
    // 获取边数
    size_t numEdges() const {
        size_t total = 0;
        for (const auto& neighbors : adj_list) {
            total += neighbors.size();
        }
        return total / 2; // 每条边被计数两次
    }
    
    // 获取顶点的邻接表
    const std::vector<std::pair<int, int>>& getNeighbors(int vertex) const {
        if (vertex < 0 || vertex >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");
        return adj_list[vertex];
    }
    
    // 1. 构建并查集 - 假设每次调用都会重建
    void offline_industry() {
        // 创建新并查集
        ds = std::make_unique<GraphDisjointSets>(adj_list.size());
        
        // 遍历所有边，合并连通的顶点
        for (size_t u = 0; u < adj_list.size(); u++) {
            for (const auto& [v, _] : adj_list[u]) {
                if (u < v) { // 只处理每条边一次
                    ds->merge(u, v);
                }
            }
        }
    }
    
    // 2. 基于并查集的可达性查询 - 假设已调用offline_industry
    bool reachability_query(int source, int target) const {
        if (source < 0 || source >= static_cast<int>(adj_list.size()) ||
            target < 0 || target >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");
            
        if (source == target)
            return true;
            
        // 使用并查集检查连通性
        return ds->find(source) == ds->find(target);
    }
    
    // 批量可达性查询 - 假设已调用offline_industry
    std::vector<bool> batch_reachability_query(int source, const std::vector<int>& targets) const {
        if (source < 0 || source >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Source vertex does not exist");
            
        std::vector<bool> results(targets.size(), false);
        int source_root = ds->find(source);
        
        for (size_t i = 0; i < targets.size(); i++) {
            int target = targets[i];
            
            if (target < 0 || target >= static_cast<int>(adj_list.size()))
                throw std::invalid_argument("Target vertex does not exist");
                
            if (target == source) {
                results[i] = true;
                continue;
            }
            
            results[i] = (ds->find(target) == source_root);
        }
        
        return results;
    }
    
    // 获取指定顶点所在的连通分量 - 假设已调用offline_industry
    std::vector<int> getConnectedComponent(int vertex) const {
        if (vertex < 0 || vertex >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");
            
        int root = ds->find(vertex);
        std::vector<int> component;
        
        for (size_t i = 0; i < adj_list.size(); i++) {
            if (ds->find(i) == root) {
                component.push_back(i);
            }
        }
        
        return component;
    }
    
    // 获取图中所有连通分量 - 假设已调用offline_industry
    std::vector<std::vector<int>> getAllConnectedComponents() const {
        // 使用映射存储每个根节点对应的连通分量
        std::vector<int> root_to_index(adj_list.size(), -1);
        std::vector<std::vector<int>> components;
        
        // 第一遍：确定每个连通分量的根节点并预分配空间
        std::vector<int> component_size(adj_list.size(), 0);
        for (size_t i = 0; i < adj_list.size(); i++) {
            int root = ds->find(i);
            component_size[root]++;
        }
        
        // 创建连通分量的索引映射
        int component_count = 0;
        for (size_t i = 0; i < adj_list.size(); i++) {
            if (component_size[i] > 0) {
                root_to_index[i] = component_count++;
                components.push_back(std::vector<int>());
                components.back().reserve(component_size[i]);
            }
        }
        
        // 第二遍：将顶点分配到对应的连通分量
        for (size_t i = 0; i < adj_list.size(); i++) {
            int root = ds->find(i);
            components[root_to_index[root]].push_back(i);
        }
        
        return components;
    }
    
private:
    // 存储图结构的邻接表
    std::vector<std::vector<std::pair<int, int>>> adj_list; // [vertex][{neighbor, weight}]
    
    // 并查集
    mutable std::unique_ptr<GraphDisjointSets> ds;
};

#endif // WEIGHTED_GRAPH_H