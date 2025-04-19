//// filepath: /root/Projects/ReachCompress/include/WeightedGraph.h
#ifndef WEIGHTED_GRAPH_H
#define WEIGHTED_GRAPH_H

#include <vector>
#include <limits>
#include <stdexcept>
#include <memory>
#include <queue>
#include <utility>
#include <numeric> // For std::accumulate
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

// 嵌入式并查集实现，专为WeightedGraph设计
class GraphDisjointSets
{
public:
    GraphDisjointSets(size_t n) : parent(n), rank(n, 0)
    {
        for (size_t i = 0; i < n; i++)
            parent[i] = i;
    }

    int find(int x)
    {
        if (parent[x] != x)
            parent[x] = find(parent[x]); // 路径压缩
        return parent[x];
    }

    void merge(int x, int y)
    {
        int root_x = find(x);
        int root_y = find(y);

        if (root_x == root_y)
            return;

        // 按秩合并
        if (rank[root_x] < rank[root_y])
            parent[root_x] = root_y;
        else if (rank[root_x] > rank[root_y])
            parent[root_y] = root_x;
        else
        {
            parent[root_y] = root_x;
            rank[root_x]++;
        }
    }

    // Make parent and rank public for memory calculation or add getters
    std::vector<int> parent;
    std::vector<int> rank;

private:

};

// 无向带权图类 - 简化版本
class WeightedGraph
{
public:

    // 构造函数
    WeightedGraph(size_t num_vertices = 0,size_t min_weight=0, size_t estimated_edges_per_vertex = 10) 
    : adj_list(num_vertices),min_weight(min_weight)
    {
        // 预分配空间以减少重新分配
        for (auto &neighbors : adj_list)
        {
            neighbors.reserve(estimated_edges_per_vertex);
        }

        // 初始化标签向量
        labels.resize(num_vertices);
    }

    // 批量添加顶点
    void addVertices(size_t count, size_t estimated_edges_per_vertex = 10)
    {
        size_t original_size = adj_list.size();
        adj_list.resize(original_size + count);

        for (size_t i = original_size; i < adj_list.size(); i++)
        {
            adj_list[i].reserve(estimated_edges_per_vertex);
        }
    }

    // 添加顶点，返回顶点ID
    int addVertex(size_t estimated_edges = 10)
    {
        adj_list.push_back(std::vector<std::pair<int, int>>());
        adj_list.back().reserve(estimated_edges);
        return adj_list.size() - 1;
    }

    // 批量添加边
    void addEdges(const std::vector<std::tuple<int, int, int>> &edges)
    {
        for (const auto &[u, v, weight] : edges)
        {
            if (u < 0 || u >= static_cast<int>(adj_list.size()) ||
                v < 0 || v >= static_cast<int>(adj_list.size()))
                throw std::invalid_argument("Vertex does not exist");

            adj_list[u].emplace_back(v, weight);
            adj_list[v].emplace_back(u, weight); // 无向图需要双向连接
        }
    }

    // 添加边及其权重
    void addEdge(int u, int v, int weight)
    {
        if (u < 0 || u >= static_cast<int>(adj_list.size()) ||
            v < 0 || v >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");

        adj_list[u].emplace_back(v, weight);
        adj_list[v].emplace_back(u, weight); // 无向图需要双向连接
    }

    // 获取顶点数
    size_t numVertices() const
    {
        return adj_list.size();
    }

    // 获取边数
    size_t numEdges() const
    {
        size_t edge_count = 0;
        for (size_t u = 0; u < adj_list.size(); ++u)
        {
            for (const auto &edge : adj_list[u])
            {
                if (u < static_cast<size_t>(edge.first))
                {
                    edge_count++;
                }
            }
        }
        return edge_count;
    }

    // 获取顶点的邻接表
    const std::vector<std::pair<int, int>> &getNeighbors(int vertex) const
    {
        if (vertex < 0 || vertex >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");
        return adj_list[vertex];
    }

    // 1. 构建并查集 - 假设每次调用都会重建
    void offline_industry()
    {
        // 创建新并查集
        ds = std::make_unique<GraphDisjointSets>(adj_list.size());

        // 遍历所有边，合并连通的顶点 (只考虑权重 >= min_weight 的边)
        for (size_t u = 0; u < adj_list.size(); u++)
        {
            for (const auto &[v, weight] : adj_list[u])
            {
                // 只处理每条边一次，并且只考虑权重符合要求的边
                if (u < static_cast<size_t>(v) && static_cast<size_t>(weight) >= min_weight)
                {
                    ds->merge(u, v);
                }
            }
        }
        // 2. 构建剪枝地标索引，先不做
        // buildPrunedLandmarkIndex();
    }

    // 2. 基于并查集的可达性查询 - 假设已调用offline_industry
    bool disjointSet_reachability_query(int source, int target) const
    {
        if (source < 0 || source >= static_cast<int>(adj_list.size()) ||
            target < 0 || target >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");

        if (source == target)
            return true;

        // 检查 ds 是否已构建
        if (!ds) {
             // 如果尚未构建，可能需要抛出错误或惰性构建
             // 这里选择抛出错误，因为查询依赖于 offline_industry
             throw std::runtime_error("Disjoint set not built. Call offline_industry first.");
        }

        // 使用并查集检查连通性
        return ds->find(source) == ds->find(target);
    }

    // 使用剪枝地标索引进行可达性查询
    bool landmark_reachability_query(int u, int v) const
    {
        if (u < 0 || u >= (int)numVertices() ||
            v < 0 || v >= (int)numVertices())
            throw std::invalid_argument("Vertex does not exist");

        if (u == v)
            return true;

        // 直接判断 labels[u] & labels[v] 是否有交集
        return intersect(labels[u], labels[v]);
    }

    // 新增：估算邻接表占用的内存（MB）
    double getAdjListMemoryUsageMB() const {
        size_t memory_bytes = 0;
        memory_bytes += sizeof(adj_list); // Outer vector object itself
        for(const auto& neighbors : adj_list) {
            memory_bytes += sizeof(neighbors); // Inner vector object itself
            // Memory for the elements (pairs) - using capacity for better estimate
            memory_bytes += neighbors.capacity() * sizeof(std::pair<int, int>);
        }
        return static_cast<double>(memory_bytes) / (1024.0 * 1024.0); // Convert bytes to MB
    }

    // 新增：估算内部并查集占用的内存（MB）
    double getDsMemoryUsageMB() const {
        size_t memory_bytes = 0;
        if (ds) {
            memory_bytes += sizeof(*ds); // Size of the DisjointSets object itself
            memory_bytes += ds->parent.capacity() * sizeof(int); // Memory for parent vector
            memory_bytes += ds->rank.capacity() * sizeof(int);   // Memory for rank vector
        }
        return static_cast<double>(memory_bytes) / (1024.0 * 1024.0); // Convert bytes to MB
    }

    // 获取总内存占用 (MB)
    double getMemoryUsageMB() const {
        return getAdjListMemoryUsageMB() + getDsMemoryUsageMB();
    }

    // 允许 Hypergraph 访问内部数据以进行内存估计
    friend class Hypergraph;
    // 允许 WeightedPrunedLandmarkIndex 访问内部数据
    friend class WeightedPrunedLandmarkIndex;

    
    // Save adjacency list to file
    bool saveAdjList(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
             std::cerr << "Error: Cannot open file for writing WeightedGraph AdjList: " << filename << std::endl;
             return false;
        }
        file << adj_list.size() << "\n"; // Number of vertices
        for (size_t u = 0; u < adj_list.size(); ++u) {
            file << u;
            for (const auto& edge : adj_list[u]) {
                file << " " << edge.first << " " << edge.second;
            }
            file << "\n";
        }
        return file.good();
    }

    // Load adjacency list from file
    bool loadAdjList(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // std::cerr << "Info: WeightedGraph AdjList cache file not found: " << filename << std::endl;
            return false;
        }
        size_t num_vertices_expected;
        file >> num_vertices_expected;
        if (file.fail() || num_vertices_expected != adj_list.size()) { // Check against current size
             std::cerr << "Error: WeightedGraph AdjList cache size mismatch or read error." << std::endl;
             return false;
        }

        // Clear existing adjacency list data before loading
        for(auto& neighbors : adj_list) {
            neighbors.clear();
        }

        std::string line;
        std::getline(file, line); // Consume rest of the first line
        int u, neighbor, weight;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            iss >> u;
             if (iss.fail() || u < 0 || u >= num_vertices_expected) {
                  std::cerr << "Warning: Skipping invalid line in WeightedGraph AdjList cache: " << line << std::endl;
                  continue; // Skip invalid lines
             }
            while (iss >> neighbor >> weight) {
                if (neighbor >= 0 && neighbor < num_vertices_expected) { // Basic validation
                    adj_list[u].emplace_back(neighbor, weight);
                } else {
                     std::cerr << "Warning: Invalid neighbor ID " << neighbor << " for vertex " << u << " in AdjList cache." << std::endl;
                }
            }
        }
         if (file.bad()) {
             std::cerr << "Error: File read error occurred during WeightedGraph AdjList cache loading." << std::endl;
             return false;
         }
        return true;
    }

    // Save Disjoint Sets state to file
    bool saveDisjointSets(const std::string& filename) const {
        if (!ds) {
             std::cerr << "Error: Cannot save WeightedGraph DisjointSets, it's not built." << std::endl;
             return false; // Nothing to save
        }
        std::ofstream file(filename);
        if (!file.is_open()) {
             std::cerr << "Error: Cannot open file for writing WeightedGraph DisjointSets: " << filename << std::endl;
             return false;
        }
        file << ds->parent.size() << "\n";
        for (size_t i = 0; i < ds->parent.size(); ++i) {
            file << ds->parent[i] << " " << ds->rank[i] << "\n";
        }
        return file.good();
    }

    // Load Disjoint Sets state from file
    bool loadDisjointSets(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // std::cerr << "Info: WeightedGraph DisjointSets cache file not found: " << filename << std::endl;
            return false;
        }
        size_t n_expected;
        file >> n_expected;
        if (file.fail() || n_expected != adj_list.size()) { // Size must match graph
             std::cerr << "Error: WeightedGraph DisjointSets cache size mismatch or read error." << std::endl;
             return false;
        }

        // Create or replace the DS object
        ds = std::make_unique<GraphDisjointSets>(n_expected);
        for (size_t i = 0; i < n_expected; ++i) {
            if (!(file >> ds->parent[i] >> ds->rank[i])) {
                 std::cerr << "Error: Failed to read parent/rank data from WeightedGraph DisjointSets cache for index " << i << "." << std::endl;
                 ds.reset(); // Clear partially loaded DS
                 return false;
            }
        }
         if (file.bad()) {
             std::cerr << "Error: File read error occurred during WeightedGraph DisjointSets cache loading." << std::endl;
             return false;
         }
        // Optional: Check for extra data at the end
        int check_eof;
        file >> check_eof;
        if (!file.eof()) {
             std::cerr << "Warning: Extra data found at the end of WeightedGraph DisjointSets cache file: " << filename << std::endl;
        }

        return true;
    }


private:


    // 获取指定顶点所在的连通分量 - 假设已调用offline_industry
    std::vector<int> getConnectedComponent(int vertex) const
    {
        if (vertex < 0 || vertex >= static_cast<int>(adj_list.size()))
            throw std::invalid_argument("Vertex does not exist");

        int root = ds->find(vertex);
        std::vector<int> component;

        for (size_t i = 0; i < adj_list.size(); i++)
        {
            if (ds->find(i) == root)
            {
                component.push_back(i);
            }
        }

        return component;
    }

    // 获取图中所有连通分量 - 假设已调用offline_industry
    std::vector<std::vector<int>> getAllConnectedComponents() const
    {
        // 使用映射存储每个根节点对应的连通分量
        std::vector<int> root_to_index(adj_list.size(), -1);
        std::vector<std::vector<int>> components;

        // 第一遍：确定每个连通分量的根节点并预分配空间
        std::vector<int> component_size(adj_list.size(), 0);
        for (size_t i = 0; i < adj_list.size(); i++)
        {
            int root = ds->find(i);
            component_size[root]++;
        }

        // 创建连通分量的索引映射
        int component_count = 0;
        for (size_t i = 0; i < adj_list.size(); i++)
        {
            if (component_size[i] > 0)
            {
                root_to_index[i] = component_count++;
                components.push_back(std::vector<int>());
                components.back().reserve(component_size[i]);
            }
        }

        // 第二遍：将顶点分配到对应的连通分量
        for (size_t i = 0; i < adj_list.size(); i++)
        {
            int root = ds->find(i);
            components[root_to_index[root]].push_back(i);
        }

        return components;
    }


    
    // 构建剪枝地标索引
    void buildPrunedLandmarkIndex()
    {

        int n = numVertices();
        
        // 清空所有标签
        for (auto &label : labels)
        {
            label.clear();
        }

        // 计算每个顶点的度
        std::vector<std::pair<int, int>> degrees(n);
        for (int i = 0; i < n; ++i)
        {
            degrees[i] = {static_cast<int>(adj_list[i].size()), i};
        }
        
        // 对度进行降序排序：度大的优先处理
        std::sort(degrees.begin(), degrees.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b)
                  { return a.first > b.first; });

        // 根据排序结果生成遍历顺序
        std::vector<int> verticesOrder(n);
        for (int i = 0; i < n; ++i)
        {
            verticesOrder[i] = degrees[i].second;
        }

        // 按照降序度序依次对每个顶点做 BFS 扩展
        for (int landmark : verticesOrder)
        {
            prunedBFS(landmark);
        }
        
        // 将每个点加入自己的标签集合
        for (int i = 0; i < n; ++i)
        {
            insertSorted(labels[i], i);
        }
    }

    // 剪枝BFS，更新标签
    void prunedBFS(int curLandmark)
    {
        // 准备一个 visited 数组，防止重复遍历
        std::vector<bool> visited(numVertices(), false);
        visited[curLandmark] = true;

        // BFS 队列
        std::queue<int> q;
        q.push(curLandmark);

        while (!q.empty())
        {
            int current = q.front();
            q.pop();

            // 若已有标签可推断 curLandmark->current 可达，则剪枝
            if (hopQuery(curLandmark, current) && current != curLandmark)
            {
                continue;
            }

            // 否则，把 curLandmark 这个标签加入 current 的 label
            if (current != curLandmark)
            {
                insertSorted(labels[current], curLandmark);
            }

            // 扩展 current 的邻居
            // 忽略 weight < threshold 的边
            const auto &neighbors = adj_list[current];
            for (auto &[nbr, w] : neighbors)
            {
                if (w >= this->min_weight && !visited[nbr])
                {
                    visited[nbr] = true;
                    q.push(nbr);
                }
            }
        }
    }

    // 判断是否可以通过已有标签推断 curLandmark 与 node 可达
    bool hopQuery(int curLandmark, int node) const
    {
        // 先特殊处理：如果 node == curLandmark 则可直接返回true
        if (node == curLandmark)
        {
            return true;
        }

        // 直接做向量交集： labels[curLandmark], labels[node]
        return intersect(labels[curLandmark], labels[node]);
    }

    // 插入值到有序向量（如果不存在）
    void insertSorted(std::vector<int> &vec, int val) const
    {
        auto it = std::lower_bound(vec.begin(), vec.end(), val);
        if (it == vec.end() || *it != val)
        {
            vec.insert(it, val);
        }
    }

    // 判断两个有序向量是否有交集
    bool intersect(const std::vector<int> &vec1, const std::vector<int> &vec2) const
    {
        int i = 0, j = 0;
        while (i < (int)vec1.size() && j < (int)vec2.size())
        {
            if (vec1[i] == vec2[j])
            {
                return true;
            }
            else if (vec1[i] < vec2[j])
            {
                i++;
            }
            else
            {
                j++;
            }
        }
        return false;
    }



    // 存储图结构的邻接表
    std::vector<std::vector<std::pair<int, int>>> adj_list; // [vertex][{neighbor, weight}]

    // 并查集
    mutable std::unique_ptr<GraphDisjointSets> ds;

    // label[v]：存储所有能使v和其他点相通的Landmark
    std::vector<std::vector<int>> labels;

    //当前图对应的最小边权重值
    size_t min_weight;
};



#endif // WEIGHTED_GRAPH_H