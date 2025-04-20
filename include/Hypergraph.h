#ifndef HYPERGRAPH_H
#define HYPERGRAPH_H
#include <thread> // 添加此行
#include <mutex>  // 添加此行
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <memory>
#include <numeric>
#include "UndirectedPLL.h"
#include "WeightedGraph.h"
#include "UWeightedPLL.h"

using namespace std;

// 单独实现并查集类
class DisjointSets
{
public:
    DisjointSets(size_t n) : parent(n), rank(n, 0)
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


    

    // Save Disjoint Sets state to file
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
             std::cerr << "Error: Cannot open file for writing DisjointSets: " << filename << std::endl;
             return false;
        }
        file << parent.size() << "\n"; // Save number of elements
        for (size_t i = 0; i < parent.size(); ++i) {
            file << parent[i] << " " << rank[i] << "\n";
        }
        return file.good();
    }

    // Load Disjoint Sets state from file
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // std::cerr << "Info: DisjointSets cache file not found: " << filename << std::endl;
            return false; // Indicate cache miss
        }
        size_t n_expected;
        file >> n_expected;
        if (file.fail()) {
             std::cerr << "Error: Failed to read size from DisjointSets cache: " << filename << std::endl;
             return false;
        }

        // Resize vectors according to file content
        parent.resize(n_expected);
        rank.resize(n_expected);

        for (size_t i = 0; i < n_expected; ++i) {
            if (!(file >> parent[i] >> rank[i])) {
                 std::cerr << "Error: Failed to read parent/rank data from DisjointSets cache for index " << i << " in file: " << filename << std::endl;
                 // Clear potentially partially loaded data
                 parent.clear();
                 rank.clear();
                 return false;
            }
        }
         if (file.bad()) {
             std::cerr << "Error: File read error occurred during DisjointSets cache loading." << std::endl;
             return false;
         }
        // Check if we read exactly n_expected elements and nothing more unexpected
        int check_eof;
        file >> check_eof;
        if (!file.eof()) {
             std::cerr << "Warning: Extra data found at the end of DisjointSets cache file: " << filename << std::endl;
             // Decide if this is an error or just a warning
        }

        return true; // Successfully loaded
    }



    std::vector<int> parent;
    std::vector<int> rank;



    
private:
};

// 表示一个超边
struct Hyperedge
{
    std::vector<int> vertices; // 顶点ID列表

    Hyperedge()
    {
        // 预分配一定容量，避免频繁重分配
        vertices.reserve(16);
    }

    Hyperedge(size_t expected_size)
    {
        vertices.reserve(expected_size);
    }

    Hyperedge(const std::vector<int> &vertices_)
        : vertices(vertices_) {}

    size_t size() const
    {
        return vertices.size();
    }

    // 计算与另一个超边的交集
    std::vector<int> intersection(const Hyperedge &other) const
    {
        std::vector<int> result;
        result.reserve(std::min(vertices.size(), other.vertices.size()));

        // 假定顶点ID有序，可以用更高效的算法
        std::vector<int> sorted_vertices = vertices;
        std::vector<int> other_sorted = other.vertices;
        std::sort(sorted_vertices.begin(), sorted_vertices.end());
        std::sort(other_sorted.begin(), other_sorted.end());

        std::set_intersection(
            sorted_vertices.begin(), sorted_vertices.end(),
            other_sorted.begin(), other_sorted.end(),
            std::back_inserter(result));
        return result;
    }
};

// 超图类 - 针对大规模数据集优化
class Hypergraph
{
public:
    // 使用预估的顶点数和超边数构造，避免频繁扩容
    Hypergraph(size_t expected_vertices = 100, size_t expected_edges = 100)
        : ds_valid(false), pll_graph(nullptr), pll(nullptr)
    {
        vertices.reserve(expected_vertices);
        hyperedges.reserve(expected_edges);
        // 直接使用vector而非unordered_map，因为顶点ID是连续的
        vertex_to_edges.reserve(expected_vertices);
    }

    // 计算两个超边的交集（返回共同的顶点）
    std::vector<int> getHyperedgeIntersection(int edgeId1, int edgeId2) const
    {
        return getHyperedge(edgeId1).intersection(getHyperedge(edgeId2));
    }

    DisjointSets buildDisjointSets() const
    {
        DisjointSets ds(vertices.size());

        // 对每个超边，将其中的所有顶点合并到同一连通分量
        for (const auto &edge : hyperedges)
        {
            if (edge.vertices.empty())
                continue;

            int first = edge.vertices[0];
            for (size_t i = 1; i < edge.vertices.size(); i++)
            {
                ds.merge(first, edge.vertices[i]);
            }
        }

        return ds;
    }

    // 添加顶点
    int addVertex()
    {
        ds_valid = false; // 数据变更，标记并查集失效
        graphs_built = false; // 重置图构建标志
        int vertexId = vertices.size();
        vertices.push_back(vertexId);
        // 为新顶点添加一个空的超边列表
        vertex_to_edges.push_back(std::vector<int>());
        return vertexId;
    }

    // 批量添加顶点，返回扩容后的顶点数量
    int addVertices(size_t count)
    {
        ds_valid = false; // 数据变更，标记并查集失效
        graphs_built = false; // 重置图构建标志
        int firstId = vertices.size();
        vertices.resize(firstId + count);
        vertex_to_edges.resize(firstId + count);
        // 初始化顶点ID
        for (size_t i = 0; i < count; i++)
        {
            vertices[firstId + i] = firstId + i;
        }
        return vertices.size();
    }

    // **新方法：通过指定顶点号添加顶点**
    void addVertexWithId(int vertexId)
    {
        ds_valid = false; // 数据变更，标记并查集失效
        graphs_built = false; // 重置图构建标志
        if (vertexId < 0)
        {
            throw std::invalid_argument("Vertex ID must be non-negative");
        }
        if (vertexId >= static_cast<int>(vertices.size()))
        {
            // 如果顶点ID超出当前范围，扩展顶点列表
            vertices.resize(vertexId + 10);
            vertex_to_edges.resize(vertexId + 10);
            for (int i = vertices.size() - 1; i >= vertexId; --i)
            {
                vertices[i] = i;
            }
        }
    }

    // **新方法：通过指定边号添加超边**
    void addHyperedgeWithId(int edgeId, const std::vector<int> &vertexList)
    {
        ds_valid = false; // 数据变更，标记并查集失效
        graphs_built = false; // 重置图构建标志
        if (edgeId < 0)
        {
            throw std::invalid_argument("Edge ID must be non-negative");
        }
        if (edgeId >= static_cast<int>(hyperedges.size()))
        {
            // 如果边ID超出当前范围，扩展超边列表
            hyperedges.resize(edgeId + 1);
        }
        hyperedges[edgeId] = Hyperedge(vertexList);

        // 更新顶点到超边的映射
        for (int v : vertexList)
        {
            if (v < 0)
            {
                throw std::invalid_argument("Vertex ID in edge does not exist");
            }else if (v >= static_cast<int>(vertex_to_edges.size()))
            {
                // 如果顶点ID超出当前范围，先添加顶点
                addVertexWithId(v);
            }
            vertex_to_edges[v].push_back(edgeId);
        }
    }

    // 添加超边，所有参与顶点必须已存在
    int addHyperedge(const std::vector<int> &vertexList)
    {
        ds_valid = false; // 数据变更，标记并查集失效
        graphs_built = false; // 重置图构建标志
        int maxVertex = vertices.size() - 1;
        for (int v : vertexList)
        {
            if (v < 0 || v > maxVertex)
                throw std::invalid_argument("Vertex id does not exist");
        }

        int edgeId = hyperedges.size();
        hyperedges.emplace_back(Hyperedge(vertexList));

        // 更新顶点到超边的映射
        for (int v : vertexList)
        {
            vertex_to_edges[v].push_back(edgeId);
        }

        return edgeId;
    }

    // 预分配一批超边容量
    void reserveHyperedges(size_t count)
    {
        hyperedges.reserve(hyperedges.size() + count);
    }

    // 查询顶点所属的超边列表
    const std::vector<int> &getIncidentHyperedges(int vertexId) const
    {
        if (vertexId < 0 || vertexId >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");
        return vertex_to_edges[vertexId];
    }

    // 获取超边
    const Hyperedge &getHyperedge(int edgeId) const
    {
        if (edgeId < 0 || edgeId >= static_cast<int>(hyperedges.size()))
            throw std::invalid_argument("Hyperedge id does not exist");
        return hyperedges[edgeId];
    }

    // 返回超图中顶点数目
    size_t numVertices() const
    {
        return vertices.size();
    }

    // 返回超图中超边数目
    size_t numHyperedges() const
    {
        return hyperedges.size();
    }

    // 从文件加载超图
    // 文件格式：每行是一个超边，包含所有顶点ID，以空格分隔
    static Hypergraph fromFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::string line;
        int max_vertex_id = -1;
        std::vector<std::vector<int>> edge_data;

        // 第一遍扫描，找出最大顶点ID并收集边数据
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            int vertex_id;
            std::vector<int> edge;

            while (iss >> vertex_id)
            {
                edge.push_back(vertex_id);
                max_vertex_id = std::max(max_vertex_id, vertex_id);
            }

            if (!edge.empty())
            {
                edge_data.push_back(std::move(edge));
            }
        }

        // 创建超图并添加顶点和边
        Hypergraph hg(max_vertex_id + 1, edge_data.size());
        hg.addVertices(max_vertex_id + 1);

        for (const auto &edge : edge_data)
        {
            hg.addHyperedge(edge);
        }

        return hg;
    }
    // 离线预处理
    // 1、构建超图并查集
    // 2、构建反转后的无向加权图，以及反转图的相关参数
    // 3、构建PLL索引
    void offline_industry(){
        void offline_industry_baseline();
        void offline_industry_pll();

    }
    void offline_industry_pll(string filename = ""){

            // --- 构建 pll_graph 和 pll index ---
            pll_graph.release();
            pll_graph = std::make_unique<WeightedGraph>(hyperedges.size());
            for (size_t i = 0; i < hyperedges.size(); i++)
            {
                if (hyperedges[i].size() == 0) continue;
                for (size_t j = i + 1; j < hyperedges.size(); j++)
                {
                    if (hyperedges[j].size() == 0) continue;
                    auto intersection = getHyperedgeIntersection(i, j);
                    int size = intersection.size();
                    if (size > 0)
                    {
                        pll_graph->addEdge(i, j, size);
                    }
                }
            }
            pll = std::make_unique<WeightedPrunedLandmarkIndex>(*pll_graph);
            pll->offline_industry(filename);
            pll_total_label_size = pll->getTotalLabelSize();
            // Store memory in bytes internally
            pll_memory_bytes = static_cast<size_t>((pll_graph->getAdjListMemoryUsageMB() + pll->getMemoryUsageMB()) * 1024.0 * 1024.0);
    }


    void offline_industry_baseline(const std::string& cache_prefix = "")
    {
        bool hg_ds_loaded = false;
        bool layered_graphs_loaded = false;

        // --- Try Loading Hypergraph DS from Cache ---
        if (!cache_prefix.empty()) {
            std::string hg_ds_file = cache_prefix + "_hg_ds.idx";
            try {
                // Create a temporary DS object to load into
                auto temp_ds = std::make_unique<DisjointSets>(0); // Initial size 0, loadFromFile will resize
                if (temp_ds->loadFromFile(hg_ds_file)) {
                    // Check if loaded size matches current vertex count
                    if (temp_ds->parent.size() == vertices.size()) {
                        ds = std::move(temp_ds); // Move ownership
                        ds_valid = true;
                        ds_nodes = ds->parent.size();
                        ds_memory_bytes = sizeof(*ds) + ds->parent.capacity() * sizeof(int) + ds->rank.capacity() * sizeof(int);
                        hg_ds_loaded = true;
                        std::cout << "Hypergraph Disjoint Set loaded from cache: " << hg_ds_file << std::endl;
                    } else {
                         std::cerr << "Warning: Hypergraph DS cache size mismatch (" << temp_ds->parent.size()
                                   << ") vs current vertex count (" << vertices.size() << "). Rebuilding DS." << std::endl;
                         ds.reset(); // Ensure ds is cleared if size mismatches
                         ds_valid = false;
                    }
                } else {
                     std::cout << "Info: Hypergraph DS cache not found or invalid at '" << hg_ds_file << "'. Building DS." << std::endl;
                }
            } catch (const std::exception& e) {
                 std::cerr << "Warning: Failed to load Hypergraph DS cache '" << hg_ds_file << "'. Building DS. Error: " << e.what() << std::endl;
                 ds.reset(); // Ensure ds is cleared on error
                 ds_valid = false;
            }
        }

        // --- Build Hypergraph DS (if not loaded) ---
        if (!hg_ds_loaded) {
            std::cout << "Building Hypergraph Disjoint Set..." << std::endl;
            buildHypergraphDS(); // Use the existing helper function

            // --- Save Hypergraph DS to Cache ---
            if (!cache_prefix.empty()) {
                std::string hg_ds_file = cache_prefix + "_hg_ds.idx";
                try {
                    std::filesystem::path p(hg_ds_file);
                    if (p.has_parent_path()) {
                        std::filesystem::create_directories(p.parent_path());
                    }
                    if (ds && ds->saveToFile(hg_ds_file)) {
                        std::cout << "Hypergraph Disjoint Set saved to cache: " << hg_ds_file << std::endl;
                    } else if (ds) {
                        // saveToFile prints error internally
                    }
                } catch (const std::exception& e) {
                     std::cerr << "Warning: Failed to save Hypergraph DS cache '" << hg_ds_file << "'. Error: " << e.what() << std::endl;
                }
            }
        }


        // --- Try Loading Layered Graphs from Cache ---
        // (This part reuses/refines the logic from the previous response)
        if (!cache_prefix.empty() && !graphs_built) { // Only try loading if not already built/loaded
             bool loaded_all_layers = true; // Assume success initially
             weighted_graphs.clear();
             weighted_graphs.resize(MAX_INTERSECTION_SIZE + 1);
             weighted_graphs_adj_list_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
             weighted_graphs_ds_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
             weighted_graphs_total_nodes = 0;
             weighted_graphs_total_edges = 0;

            for (int k = 1; k <= MAX_INTERSECTION_SIZE; ++k) {
                std::string adj_file = cache_prefix + "_lds_k" + std::to_string(k) + "_adj.idx";
                std::string ds_file = cache_prefix + "_lds_k" + std::to_string(k) + "_ds.idx";

                // Create graph object first, passing expected size and min_weight
                weighted_graphs[k] = std::make_unique<WeightedGraph>(hyperedges.size(), k);

                if (!weighted_graphs[k]->loadAdjList(adj_file) || !weighted_graphs[k]->loadDisjointSets(ds_file)) {
                    std::cout << "Info: Layered DS cache not found or invalid for k=" << k << ". Building required." << std::endl;
                    loaded_all_layers = false;
                    weighted_graphs.clear(); // Clear partially loaded graphs
                    // Reset stats
                    weighted_graphs_adj_list_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
                    weighted_graphs_ds_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
                    weighted_graphs_total_nodes = 0;
                    weighted_graphs_total_edges = 0;
                    break; // Stop trying to load
                }
                 // Update stats after successful load for layer k
                 weighted_graphs_total_nodes += weighted_graphs[k]->numVertices();
                 weighted_graphs_total_edges += weighted_graphs[k]->numEdges();
                 weighted_graphs_adj_list_memory_bytes[k] = static_cast<size_t>(weighted_graphs[k]->getAdjListMemoryUsageMB() * 1024.0 * 1024.0);
                 weighted_graphs_ds_memory_bytes[k] = static_cast<size_t>(weighted_graphs[k]->getDsMemoryUsageMB() * 1024.0 * 1024.0);
            }
            if (loaded_all_layers) {
                 std::cout << "All Layered DS levels loaded from cache prefix: " << cache_prefix << std::endl;
                 graphs_built = true; // Mark as built (loaded)
                 layered_graphs_loaded = true;
                 // No need to return here, let the function finish naturally
            }
        }


        // --- Build Layered Graphs (if not loaded) ---
        if (!layered_graphs_loaded && !graphs_built) // Check graphs_built again in case it was set elsewhere
        {
            std::cout << "Building Layered DS..." << std::endl;

            // Calculate intersections (parallel) - Reuse existing logic if needed
            if (all_intersections.empty()) { // Only calculate if not already done
                 calculateAllIntersectionsParallel();
            }

            // Build graphs sequentially
            weighted_graphs.clear();
            weighted_graphs.resize(MAX_INTERSECTION_SIZE + 1);
            weighted_graphs_adj_list_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
            weighted_graphs_ds_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
            weighted_graphs_total_nodes = 0;
            weighted_graphs_total_edges = 0;

            for (int min_size = 1; min_size <= MAX_INTERSECTION_SIZE; min_size++) {
                // Create graph object with correct size and min_weight
                weighted_graphs[min_size] = std::make_unique<WeightedGraph>(hyperedges.size(), min_size);

                // Add edges from pre-calculated intersections
                for (const auto& intersection_info : all_intersections) {
                    if (std::get<2>(intersection_info) >= min_size) {
                        weighted_graphs[min_size]->addEdge(std::get<0>(intersection_info), std::get<1>(intersection_info), std::get<2>(intersection_info));
                    }
                }

                // Build DS for this graph level
                weighted_graphs[min_size]->offline_industry();

                // Update stats
                weighted_graphs_total_nodes += weighted_graphs[min_size]->numVertices();
                weighted_graphs_total_edges += weighted_graphs[min_size]->numEdges();
                weighted_graphs_adj_list_memory_bytes[min_size] = static_cast<size_t>(weighted_graphs[min_size]->getAdjListMemoryUsageMB() * 1024.0 * 1024.0);
                weighted_graphs_ds_memory_bytes[min_size] = static_cast<size_t>(weighted_graphs[min_size]->getDsMemoryUsageMB() * 1024.0 * 1024.0);

                // --- Save Layer to Cache ---
                if (!cache_prefix.empty()) {
                     std::string adj_file = cache_prefix + "_lds_k" + std::to_string(min_size) + "_adj.idx";
                     std::string ds_file = cache_prefix + "_lds_k" + std::to_string(min_size) + "_ds.idx";
                     try {
                         std::filesystem::path p(adj_file);
                         if (p.has_parent_path()) {
                             std::filesystem::create_directories(p.parent_path());
                         }
                         // Save both adjacency list and DS state
                         if (!weighted_graphs[min_size]->saveAdjList(adj_file)) {
                              std::cerr << "Warning: Failed to save Layered DS adjacency list cache for k=" << min_size << std::endl;
                         }
                         if (!weighted_graphs[min_size]->saveDisjointSets(ds_file)) {
                              std::cerr << "Warning: Failed to save Layered DS disjoint set cache for k=" << min_size << std::endl;
                         }
                     } catch (const std::exception& e) {
                          std::cerr << "Warning: Failed to save Layered DS cache for k=" << min_size << ". Error: " << e.what() << std::endl;
                     }
                }
            }
            graphs_built = true; // Mark as built
            if (!cache_prefix.empty()) { // Only print save message if we actually built and had a path
                 std::cout << "Layered DS saved to cache prefix: " << cache_prefix << std::endl;
            }
        }
    }

    // void offline_industry_baseline()
    // {
    //     // 检查是否需要重建并查集 (Hypergraph level) - 这部分保持不变
    //     if (!ds_valid || !ds)
    //     {
    //         ds = std::make_unique<DisjointSets>(vertices.size());
    //         for (const auto &edge : hyperedges)
    //         {
    //             if (edge.vertices.size() > 1)
    //             {
    //                 int first = edge.vertices[0];
    //                 for (size_t i = 1; i < edge.vertices.size(); i++)
    //                 {
    //                     ds->merge(first, edge.vertices[i]);
    //                 }
    //             }
    //         }
    //         ds_valid = true;
    //         ds_nodes = ds->parent.size();
    //         ds_memory_bytes = sizeof(*ds) + ds->parent.capacity() * sizeof(int) + ds->rank.capacity() * sizeof(int);
    //     }

    //     // --- 多线程计算所有交集 ---
    //     if (!graphs_built) // 仅在未构建时执行
    //     {
    //         all_intersections.clear(); // 清空之前的交集数据
    //         std::mutex intersections_mutex; // 用于保护 all_intersections 的互斥锁
    //         size_t num_edges = hyperedges.size();
    //         unsigned int num_threads = std::thread::hardware_concurrency(); // 获取硬件支持的线程数
    //         if (num_threads == 0) num_threads = 1; // 至少使用一个线程
    //         std::vector<std::thread> threads(num_threads);

    //         auto calculate_intersections =
    //             [&](size_t start_idx, size_t end_idx) {
    //             std::vector<std::tuple<size_t, size_t, int>> local_intersections; // 线程局部结果
    //             for (size_t i = start_idx; i < end_idx; ++i) {
    //                 if (hyperedges[i].size() == 0) continue;
    //                 for (size_t j = i + 1; j < num_edges; ++j) {
    //                      if (hyperedges[j].size() == 0) continue;
    //                      // 注意：getHyperedgeIntersection 内部有排序，可能较慢
    //                      // 如果性能关键，可以考虑优化交集计算本身
    //                      auto intersection = getHyperedgeIntersection(i, j);
    //                      int size = intersection.size();
    //                      if (size > 0) { // 只存储有交集的边
    //                          local_intersections.emplace_back(i, j, size);
    //                      }
    //                 }
    //             }
    //             // 将局部结果合并到全局结果中（需要加锁）
    //             std::lock_guard<std::mutex> lock(intersections_mutex);
    //             all_intersections.insert(all_intersections.end(),
    //                                      local_intersections.begin(),
    //                                      local_intersections.end());
    //         };

    //         size_t total_pairs = num_edges * (num_edges - 1) / 2; // 大致的总对数，用于分配任务
    //         size_t chunk_size = (num_edges + num_threads -1) / num_threads; // 粗略按第一个索引划分任务量

    //         size_t current_start = 0;
    //         for (unsigned int t = 0; t < num_threads; ++t) {
    //             size_t current_end = std::min(current_start + chunk_size, num_edges);
    //              if (current_start >= current_end) break; // 防止创建空任务的线程
    //             threads[t] = std::thread(calculate_intersections, current_start, current_end);
    //             current_start = current_end;
    //         }

    //         // 等待所有线程完成
    //         for (unsigned int t = 0; t < threads.size(); ++t) {
    //              if (threads[t].joinable()) {
    //                 threads[t].join();
    //              }
    //         }

    //         // --- 顺序构建各层图 ---
    //         weighted_graphs.clear();
    //         weighted_graphs.resize(MAX_INTERSECTION_SIZE + 1);
    //         weighted_graphs_adj_list_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
    //         weighted_graphs_ds_memory_bytes.assign(MAX_INTERSECTION_SIZE + 1, 0);
    //         weighted_graphs_total_nodes = 0;
    //         weighted_graphs_total_edges = 0;

    //         for (int min_size = 1; min_size <= MAX_INTERSECTION_SIZE; min_size++)
    //         {
    //             weighted_graphs[min_size] = std::make_unique<WeightedGraph>(num_edges, min_size);

    //             // 遍历预计算的交集
    //             for (const auto& intersection_info : all_intersections)
    //             {
    //                 size_t i = std::get<0>(intersection_info);
    //                 size_t j = std::get<1>(intersection_info);
    //                 int size = std::get<2>(intersection_info);

    //                 // 如果交集大小满足当前层的要求，则添加边
    //                 if (size >= min_size)
    //                 {
    //                     weighted_graphs[min_size]->addEdge(i, j, size);
    //                 }
    //             }

    //             // 为当前层构建索引并更新统计信息
    //             weighted_graphs[min_size]->offline_industry(); // Build DS for this graph level
    //             weighted_graphs_total_nodes += weighted_graphs[min_size]->numVertices();
    //             weighted_graphs_total_edges += weighted_graphs[min_size]->numEdges();
    //             weighted_graphs_adj_list_memory_bytes[min_size] = static_cast<size_t>(weighted_graphs[min_size]->getAdjListMemoryUsageMB() * 1024.0 * 1024.0);
    //             weighted_graphs_ds_memory_bytes[min_size] = static_cast<size_t>(weighted_graphs[min_size]->getDsMemoryUsageMB() * 1024.0 * 1024.0);
    //         }
    //         graphs_built = true;
    //     }
    // }


    // 检查两个顶点是否连通（可达）- 优化使用缓存的并查集
    bool isReachable(int u, int v) const
    {
        if (u < 0 || u >= static_cast<int>(vertices.size()) ||
            v < 0 || v >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 如果有有效的并查集，就使用它
        if (ds_valid && ds)
        {
            return ds->find(u) == ds->find(v);
        }
        else
        {
            // 否则创建临时的并查集
            DisjointSets temp_ds = buildDisjointSets();
            return temp_ds.find(u) == temp_ds.find(v);
        }
    }

    bool isReachableBidirectionalBFS(int source, int target)
    {
        if (source < 0 || source >= static_cast<int>(vertices.size()) ||
            target < 0 || target >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 如果是同一个顶点，则可达
        if (source == target)
            return true;

        // 两个方向的访问标记
        std::vector<bool> visited_forward(vertices.size(), false);
        std::vector<bool> visited_backward(vertices.size(), false);

        // 两个队列，分别从源点和终点开始搜索
        std::queue<int> q_forward;
        std::queue<int> q_backward;

        // 初始化
        q_forward.push(source);
        q_backward.push(target);
        visited_forward[source] = true;
        visited_backward[target] = true;

        // 当两个方向的搜索都没有结束时，继续搜索
        while (!q_forward.empty() && !q_backward.empty())
        {
            // 从源点方向扩展一层
            int forward_size = q_forward.size();
            for (int i = 0; i < forward_size; i++)
            {
                int current = q_forward.front();
                q_forward.pop();

                // 遍历当前顶点所属的所有超边
                for (int edgeId : vertex_to_edges[current])
                {
                    const auto &edge = hyperedges[edgeId];

                    // 遍历该超边中的所有顶点
                    for (int neighbor : edge.vertices)
                    {
                        // 检查是否遇到另一个方向的搜索
                        if (visited_backward[neighbor])
                        {
                            return true; // 找到路径
                        }

                        // 标记并入队新发现的顶点
                        if (!visited_forward[neighbor])
                        {
                            visited_forward[neighbor] = true;
                            q_forward.push(neighbor);
                        }
                    }
                }
            }

            // 从目标点方向扩展一层
            int backward_size = q_backward.size();
            for (int i = 0; i < backward_size; i++)
            {
                int current = q_backward.front();
                q_backward.pop();

                // 遍历当前顶点所属的所有超边
                for (int edgeId : vertex_to_edges[current])
                {
                    const auto &edge = hyperedges[edgeId];

                    // 遍历该超边中的所有顶点
                    for (int neighbor : edge.vertices)
                    {
                        // 检查是否遇到另一个方向的搜索
                        if (visited_forward[neighbor])
                        {
                            return true; // 找到路径
                        }

                        // 标记并入队新发现的顶点
                        if (!visited_backward[neighbor])
                        {
                            visited_backward[neighbor] = true;
                            q_backward.push(neighbor);
                        }
                    }
                }
            }
        }

        // 如果搜索结束还没有找到相遇点，则不可达
        return false;
    }
    bool isReachableBidirectionalBFS(int source, int target, int minIntersectionSize = 0) const
    {
        if (source < 0 || source >= static_cast<int>(vertices.size()) ||
            target < 0 || target >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 如果是同一个顶点，则可达
        if (source == target)
            return true;

        // 检查两点是否属于同一超边（直接可达）
        const auto &source_edges = vertex_to_edges[source];
        const auto &target_edges = vertex_to_edges[target];

        for (int edge_id : source_edges)
        {
            if (std::find(target_edges.begin(), target_edges.end(), edge_id) != target_edges.end())
            {
                return true; // 两点属于同一超边，直接可达
            }
        }

        // 如果没有交集约束，使用普通BFS
        if (minIntersectionSize <= 0)
            return isReachableBidirectionalBFS(source, target);

        // 记录每个顶点"前驱边"和"距离"
        std::vector<int> pred_edge_forward(vertices.size(), -1);
        std::vector<int> pred_edge_backward(vertices.size(), -1);
        std::vector<int> distance_forward(vertices.size(), -1);
        std::vector<int> distance_backward(vertices.size(), -1);

        // 两个队列，分别从源点和终点开始搜索
        std::queue<int> q_forward;
        std::queue<int> q_backward;

        // 初始化
        q_forward.push(source);
        q_backward.push(target);
        distance_forward[source] = 0;
        distance_backward[target] = 0;

        // 已发现的可能连接路径，格式为 (中间点, 前向边, 后向边)
        std::vector<std::tuple<int, int, int>> meeting_points;

        // 当两个方向的搜索都没有结束时，继续搜索
        while (!q_forward.empty() && !q_backward.empty())
        {
            // 从源点方向扩展一层
            int forward_size = q_forward.size();
            for (int i = 0; i < forward_size; i++)
            {
                int current = q_forward.front();
                q_forward.pop();

                // 遍历当前顶点所属的所有超边
                for (int edge_id : vertex_to_edges[current])
                {
                    const auto &edge = hyperedges[edge_id];

                    // 只处理满足交集约束的边
                    if (pred_edge_forward[current] == -1 || // 源点特殊处理
                        getHyperedgeIntersection(pred_edge_forward[current], edge_id).size() >=
                            static_cast<size_t>(minIntersectionSize))
                    {

                        // 遍历该超边中的所有顶点
                        for (int next : edge.vertices)
                        {
                            // 如果顶点未被访问过或找到更好的路径
                            if (distance_forward[next] == -1)
                            {
                                distance_forward[next] = distance_forward[current] + 1;
                                pred_edge_forward[next] = edge_id;
                                q_forward.push(next);

                                // 检查是否遇到后向搜索
                                if (distance_backward[next] != -1)
                                {
                                    meeting_points.emplace_back(next, edge_id, pred_edge_backward[next]);
                                }
                            }
                        }
                    }
                }
            }

            // 从目标点方向扩展一层
            int backward_size = q_backward.size();
            for (int i = 0; i < backward_size; i++)
            {
                int current = q_backward.front();
                q_backward.pop();

                // 遍历当前顶点所属的所有超边
                for (int edge_id : vertex_to_edges[current])
                {
                    const auto &edge = hyperedges[edge_id];

                    // 只处理满足交集约束的边
                    if (pred_edge_backward[current] == -1 || // 目标点特殊处理
                        getHyperedgeIntersection(pred_edge_backward[current], edge_id).size() >=
                            static_cast<size_t>(minIntersectionSize))
                    {

                        // 遍历该超边中的所有顶点
                        for (int next : edge.vertices)
                        {
                            // 如果顶点未被访问过或找到更好的路径
                            if (distance_backward[next] == -1)
                            {
                                distance_backward[next] = distance_backward[current] + 1;
                                pred_edge_backward[next] = edge_id;
                                q_backward.push(next);

                                // 检查是否遇到前向搜索
                                if (distance_forward[next] != -1)
                                {
                                    meeting_points.emplace_back(next, pred_edge_forward[next], edge_id);
                                }
                            }
                        }
                    }
                }
            }

            // 检查是否找到有效路径
            for (const auto &[meet_point, forward_edge, backward_edge] : meeting_points)
            {
                // 特殊情况：起点或终点直接可达
                if (forward_edge == -1 || backward_edge == -1)
                {
                    continue; // 需要检查下一个可能的路径
                }

                // 检查交集约束
                auto intersection = getHyperedgeIntersection(forward_edge, backward_edge);
                if (intersection.size() >= static_cast<size_t>(minIntersectionSize))
                {
                    return true; // 找到有效路径
                }
            }

            // 清空已检查的相交点
            meeting_points.clear();
        }

        // 搜索结束仍未找到有效路径
        return false;
    }

    // 使用转换后的带权图进行顶点可达性查询
    bool isReachableViaPLLWeightedGraph(int sourceVertex, int targetVertex, int minIntersectionSize = 0)
    {
        if (sourceVertex < 0 || sourceVertex >= static_cast<int>(vertices.size()) ||
            targetVertex < 0 || targetVertex >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 同一顶点必然可达
        if (sourceVertex == targetVertex)
            return true;

        // 确保图已构建，并且使用正确的交集约束级别
        if (!graphs_built || minIntersectionSize > MAX_INTERSECTION_SIZE)
        {
            const_cast<Hypergraph *>(this)->offline_industry();
        }

        // 调整交集约束到有效范围
        int effective_min_size = minIntersectionSize;

        // 获取源顶点和目标顶点关联的所有超边
        const auto &source_edges = vertex_to_edges[sourceVertex];
        const auto &target_edges = vertex_to_edges[targetVertex];

        if (source_edges.empty() || target_edges.empty())
        {
            return false; // 如果源点或终点没有关联的超边，不可达
        }

        // 如果源点和目标点有共同的超边，则直接可达
        for (int source_edge : source_edges)
        {
            if (std::find(target_edges.begin(), target_edges.end(), source_edge) != target_edges.end())
            {
                return true;
            }
        }

        // 对源点集和目标点集做笛卡尔积检查
        for (int source_edge : source_edges)
        {
            for (int target_edge : target_edges)
            {
                // 使用预构建的对应交集约束级别的图进行查询
                if (weighted_graphs[effective_min_size]->landmark_reachability_query(source_edge, target_edge))
                {
                    return true; // 找到一对可达的超边
                }
            }
        }

        return false; // 所有笛卡尔积对都不可达
    }

    bool isReachableViaWeightedGraph(int sourceVertex, int targetVertex, int minIntersectionSize = 1) // Default to 1
    {
        if (sourceVertex < 0 || sourceVertex >= static_cast<int>(vertices.size()) ||
            targetVertex < 0 || targetVertex >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 同一顶点必然可达
        if (sourceVertex == targetVertex)
            return true;

        // 确保图已构建
        if (!graphs_built)
        {
             const_cast<Hypergraph *>(this)->offline_industry();
        }

        // 处理 minIntersectionSize 的有效范围
        int effective_min_size = minIntersectionSize;
        if (effective_min_size <= 0) {
            // 如果 k=0，使用限制最宽松的层，即 k=1
            effective_min_size = 1;
        }
        if (effective_min_size > MAX_INTERSECTION_SIZE) {
             // 如果 k 超出最大构建层，理论上不可达，除非直接相连
             // 或者可以抛出错误，或使用最大层？这里选择使用最大层
             effective_min_size = MAX_INTERSECTION_SIZE;
             // 或者: throw std::out_of_range("minIntersectionSize exceeds MAX_INTERSECTION_SIZE");
        }

        // 检查请求的层是否已构建 (层 1 到 MAX_INTERSECTION_SIZE)
        if (effective_min_size < 1 || effective_min_size > MAX_INTERSECTION_SIZE || !weighted_graphs[effective_min_size]) {
             // 如果请求的层无效或未构建（例如 k=0 时层 1 未构建），则可能需要重新构建或抛出错误
             // 假设 offline_industry 总是构建 1 到 MAX_INTERSECTION_SIZE
             // 如果 offline_industry 失败，这里可能访问 nullptr
             // 增加检查确保图存在
             if (!weighted_graphs[effective_min_size]) {
                  throw std::runtime_error("Requested WeightedGraph layer " + std::to_string(effective_min_size) + " is not built.");
             }
        }


        // 获取源顶点和目标顶点关联的所有超边
        const auto &source_edges = vertex_to_edges[sourceVertex];
        const auto &target_edges = vertex_to_edges[targetVertex];

        if (source_edges.empty() || target_edges.empty())
        {
            return false; // 如果源点或终点没有关联的超边，不可达
        }

        // 如果源点和目标点有共同的超边，则直接可达
        for (int source_edge : source_edges)
        {
            if (std::find(target_edges.begin(), target_edges.end(), source_edge) != target_edges.end())
            {
                return true;
            }
        }

        // 对源点集和目标点集做笛卡尔积检查
        for (int source_edge : source_edges)
        {
            for (int target_edge : target_edges)
            {
                // 使用预构建的对应交集约束级别的图进行查询
                if (weighted_graphs[effective_min_size]->disjointSet_reachability_query(source_edge, target_edge))
                {
                    return true; // 找到一对可达的超边
                }
            }
        }

        return false; // 所有笛卡尔积对都不可达
    }

    // **新增**: 使用 UWeightedPLL 索引进行顶点可达性查询
    bool isReachableViaUWeightedPLL(int sourceVertex, int targetVertex, int minIntersectionSize = 0) const
    {
        if (sourceVertex < 0 || sourceVertex >= static_cast<int>(vertices.size()) ||
            targetVertex < 0 || targetVertex >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        // 同一顶点必然可达
        if (sourceVertex == targetVertex)
            return true;

        // 确保 PLL 索引已构建
        if (!graphs_built || !pll)
        {
            const_cast<Hypergraph *>(this)->offline_industry();
            if (!pll)
            {
                throw std::runtime_error("PLL index failed to build.");
            }
        }

        // 获取源顶点和目标顶点关联的所有超边
        const auto &source_edges = vertex_to_edges[sourceVertex];
        const auto &target_edges = vertex_to_edges[targetVertex];

        if (source_edges.empty() || target_edges.empty())
        {
            return false; // 如果源点或终点没有关联的超边，不可达
        }

        // 如果源点和目标点有共同的超边，则直接可达
        for (int source_edge : source_edges)
        {
            if (std::find(target_edges.begin(), target_edges.end(), source_edge) != target_edges.end())
            {
                return true;
            }
        }

        // 对源点集和目标点集做笛卡尔积检查，调用 UWeightedPLL 的查询
        for (int source_edge : source_edges)
        {
            for (int target_edge : target_edges)
            {
                if (pll->reachability_query(source_edge, target_edge, minIntersectionSize))
                {
                    return true; // 找到一对可通过 UWeightedPLL 确认可达的超边
                }
            }
        }

        return false; // 所有笛卡尔积对都不可达
    }

    // 计算所有连通分量 - 优化使用缓存的并查集
    std::vector<std::vector<int>> getConnectedComponents()
    {
        // 首先尝试使用缓存的并查集，如果无效则创建新的
        DisjointSets *working_ds = nullptr;
        DisjointSets temp_ds(vertices.size());

        if (ds_valid && ds)
        {
            // 使用缓存的并查集
            working_ds = ds.get();
        }
        else
        {
            // 创建临时并查集
            temp_ds = buildDisjointSets();
            working_ds = &temp_ds;
        }

        // 第一次遍历：计算每个连通分量的大小
        std::vector<int> component_size(vertices.size(), 0);
        for (size_t i = 0; i < vertices.size(); i++)
        {
            int root = working_ds->find(i);
            component_size[root]++;
        }

        // 计算有多少个非空连通分量
        int num_components = 0;
        for (size_t i = 0; i < vertices.size(); i++)
        {
            if (component_size[i] > 0)
            {
                num_components++;
            }
        }

        // 预先分配结果数组空间
        std::vector<std::vector<int>> result(num_components);
        std::vector<int> component_index(vertices.size(), -1);

        // 为每个连通分量预分配正确的大小
        int curr_idx = 0;
        for (size_t i = 0; i < vertices.size(); i++)
        {
            if (component_size[i] > 0)
            {
                component_index[i] = curr_idx;
                result[curr_idx].reserve(component_size[i]);
                curr_idx++;
            }
        }

        // 第二次遍历：将顶点分配到正确的连通分量
        for (size_t i = 0; i < vertices.size(); i++)
        {
            int root = working_ds->find(i);
            result[component_index[root]].push_back(i);
        }

        return result;
    }

    // 转换超图为无向带权图（顶点是超边，边是共享顶点）
    std::unique_ptr<WeightedGraph> convertToWeightedGraph() const
    {
        // 创建无向带权图，顶点数量等于超边数量
        auto graph = std::make_unique<WeightedGraph>(hyperedges.size());

        // 对每对超边检查交集
        for (size_t i = 0; i < hyperedges.size(); i++)
        {
            for (size_t j = i + 1; j < hyperedges.size(); j++)
            {
                // 计算共享顶点
                auto intersection = getHyperedgeIntersection(i, j);
                if (!intersection.empty())
                {
                    // 添加边，权重为共享顶点数
                    graph->addEdge(i, j, intersection.size());
                }
            }
        }

        return graph;
    }

    // 查询原始超图顶点在转换后图中对应的边集合
    std::vector<std::pair<int, int>> getVertexToGraphEdges(int vertexId, const WeightedGraph &graph) const
    {
        if (vertexId < 0 || vertexId >= static_cast<int>(vertices.size()))
            throw std::invalid_argument("Vertex id does not exist");

        std::vector<std::pair<int, int>> result;

        // 获取包含该顶点的所有超边
        const auto &incident_edges = vertex_to_edges[vertexId];

        // 对每对包含该顶点的超边，查找对应的图边
        for (size_t i = 0; i < incident_edges.size(); i++)
        {
            for (size_t j = i + 1; j < incident_edges.size(); j++)
            {
                int edge1 = incident_edges[i];
                int edge2 = incident_edges[j];

                // 这些超边对应无向图中的顶点，它们之间应该有边
                result.emplace_back(edge1, edge2);
            }
        }

        return result;
    }

    // 查询原始超图超边在转换后图中对应的顶点
    int getHyperedgeToGraphVertex(int edgeId) const
    {
        if (edgeId < 0 || edgeId >= static_cast<int>(hyperedges.size()))
            throw std::invalid_argument("Hyperedge id does not exist");

        // 在转换后的图中，原始超图的超边ID直接对应图中的顶点ID
        return edgeId;
    }

    // --- 新增 Getter 方法用于获取索引大小和内存 ---
    size_t getDsNodes() const {
        return ds_nodes;
    }

    size_t getPllTotalLabelSize() const {
        return pll_total_label_size;
    }

    size_t getWeightedGraphsTotalNodes() const {
        return weighted_graphs_total_nodes;
    }

    size_t getWeightedGraphsTotalEdges() const {
        return weighted_graphs_total_edges;
    }

    double getDsMemoryUsageMB() const {
        return static_cast<double>(ds_memory_bytes) / (1024.0 * 1024.0);
    }

    // 新增：获取指定层 WeightedGraph 邻接表的内存 (MB)
    double getWeightedGraphAdjListMemoryUsageMB(int layer) const {
        if (layer < 0 || layer > MAX_INTERSECTION_SIZE || layer >= weighted_graphs_adj_list_memory_bytes.size()) {
            return 0.0; // 或者抛出异常
        }
        return static_cast<double>(weighted_graphs_adj_list_memory_bytes[layer]) / (1024.0 * 1024.0);
    }

    // 新增：获取指定层 WeightedGraph 并查集的内存 (MB)
    double getWeightedGraphDsMemoryUsageMB(int layer) const {
         if (layer < 0 || layer > MAX_INTERSECTION_SIZE || layer >= weighted_graphs_ds_memory_bytes.size()) {
            return 0.0; // 或者抛出异常
        }
        return static_cast<double>(weighted_graphs_ds_memory_bytes[layer]) / (1024.0 * 1024.0);
    }

    // 获取所有 WeightedGraph 层总内存 (MB)
    double getWeightedGraphsMemoryUsageMB() const {
        size_t total_bytes = 0;
        for(size_t bytes : weighted_graphs_adj_list_memory_bytes) {
            total_bytes += bytes;
        }
        for(size_t bytes : weighted_graphs_ds_memory_bytes) {
            total_bytes += bytes;
        }
        return static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    }

    double getPllMemoryUsageMB() const {
        return static_cast<double>(pll_memory_bytes) / (1024.0 * 1024.0);
    }
    // --- 结束新增 Getter 方法 ---


    
    // --- 顶点删除 (直接删除，处理空洞) ---
    void deleteVertex(int vertexId) {
        // 1. 输入验证: 检查 ID 是否在范围内，以及该顶点是否实际存在
        //    (这里假设顶点存在当且仅当 vertex_to_edges[vertexId] 非空，
        //     或者如果 vertices[vertexId] 有特殊标记也可以用那个)
        if (vertexId < 0 || vertexId >= static_cast<int>(vertex_to_edges.size())) {
            throw std::invalid_argument("Vertex id is out of bounds for deletion");
        }
        // 检查顶点是否已经 "不存在" 或已被删除
        // 如果 vertices[vertexId] == 0 表示不存在，可以加上这个检查
        // if (vertices[vertexId] == 0) { // 假设 0 表示不存在
        //     return; // Vertex already deleted or never existed
        // }
        // 或者，如果 vertex_to_edges 为空表示不存在/已删除
        if (vertex_to_edges[vertexId].empty() && /* 检查 vertices[vertexId] 是否也表示不存在 */ true) {
             // 可以选择静默返回或抛出异常，表明顶点不存在或已被删除
             // throw std::invalid_argument("Vertex id does not exist or is already deleted");
             return; // Or simply return if deleting a non-existent vertex is acceptable
        }




        // 3. 更新超边列表 (hyperedges)
        //    从包含 vertexId 的超边中移除 vertexId
        //    在清除 vertex_to_edges 之前获取列表副本或直接迭代
        std::vector<int> incident_edges_copy = vertex_to_edges[vertexId]; // 获取副本
        for (int edgeId : incident_edges_copy) {
            if (edgeId >= 0 && edgeId < static_cast<int>(hyperedges.size())) { // 边界检查
                auto& edge_verts = hyperedges[edgeId].vertices;
                // 使用 erase-remove idiom 从超边的顶点列表中移除 vertexId
                edge_verts.erase(std::remove(edge_verts.begin(), edge_verts.end(), vertexId), edge_verts.end());

                // 可选: 检查超边是否因此变空或过小。
                // 如果需要处理无效超边，可以在这里添加逻辑，
                // 例如，如果 edge_verts.empty()，则标记 hyperedges[edgeId] 为无效
                // (需要为 Hyperedge 添加状态或使用单独的 hyperedge_active_ 向量)
                // 这会使问题更复杂，因为超边删除也需要处理 ID 映射。
            }
        }

        // 4. 标记顶点为已删除
        //    a. 清空该顶点的关联超边列表
        vertex_to_edges[vertexId].clear();
        //    b. (可选) 如果 vertices 向量用于标记存在性，更新它
        //       例如，如果 vertices[i] 存储 ID i，可能不需要改动，
        //       或者如果用 0 表示空洞，则设置为 0。
        // vertices[vertexId] = 0; // 假设 0 表示空洞/不存在

        // 注意：此方法不会缩小 vertices 或 vertex_to_edges 向量的大小，
        // 只是在 vertexId 处创建了一个 "逻辑空洞"。
        // 依赖于顶点迭代的方法 (如 buildDisjointSets, offline_industry)
        // 未来可能需要检查顶点是否有效（例如检查 vertex_to_edges[i].empty()）

        // 5. 更新索引
        //    a. 更新并查集 (如果使用)
        // if (ds_valid && ds) {
        //     ds_valid = false;
        //     ds->remove(vertexId);
        // }
        // //    PLL索引更新
        // if (pll) {
        //     pll->updateVertex(vertexId);
        // }
        // //    各层WeightedGraph索引更新
        // if (graphs_built) {
        //     for (auto& graph : weighted_graphs) {
        //         if (graph) {
        //             graph->removeVertex(vertexId);
        //         }
        //     }
        // }

    }

    // Helper to build hypergraph's own DS (ensure this exists and is correct)
    void buildHypergraphDS() {
        ds = std::make_unique<DisjointSets>(vertices.size());
        for (const auto &edge : hyperedges) {
            if (edge.vertices.size() > 1) {
                int first = edge.vertices[0];
                for (size_t i = 1; i < edge.vertices.size(); i++) {
                    ds->merge(first, edge.vertices[i]);
                }
            }
        }
        ds_valid = true;
        ds_nodes = ds->parent.size();
        ds_memory_bytes = sizeof(*ds) + ds->parent.capacity() * sizeof(int) + ds->rank.capacity() * sizeof(int);
   }

   // Helper to calculate all intersections in parallel (ensure this exists and is correct)
   void calculateAllIntersectionsParallel() {
       // ... (Implementation from previous response) ...
        if (!all_intersections.empty()) return; // Already calculated

        std::cout << "Calculating hyperedge intersections..." << std::endl;
        std::mutex intersections_mutex;
        size_t num_edges = hyperedges.size();
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 1;
        std::vector<std::thread> threads(num_threads);
        all_intersections.reserve(num_edges * num_edges / 20); // Heuristic preallocation

        auto calculate_intersections_task =
            [&](size_t start_idx, size_t end_idx) {
            std::vector<std::tuple<size_t, size_t, int>> local_intersections;
            local_intersections.reserve((end_idx - start_idx) * num_edges / 20); // Heuristic
            for (size_t i = start_idx; i < end_idx; ++i) {
                if (hyperedges[i].size() == 0) continue;
                for (size_t j = i + 1; j < num_edges; ++j) {
                    if (hyperedges[j].size() == 0) continue;
                    auto intersection = getHyperedgeIntersection(i, j);
                    int size = intersection.size();
                    if (size > 0) {
                        local_intersections.emplace_back(i, j, size);
                    }
                }
            }
            std::lock_guard<std::mutex> lock(intersections_mutex);
            all_intersections.insert(all_intersections.end(),
                                     local_intersections.begin(),
                                     local_intersections.end());
        };

        size_t chunk_size = (num_edges + num_threads - 1) / num_threads;
        size_t current_start = 0;
        for (unsigned int t = 0; t < num_threads; ++t) {
            size_t current_end = std::min(current_start + chunk_size, num_edges);
            if (current_start >= current_end) break;
            threads[t] = std::thread(calculate_intersections_task, current_start, current_end);
            current_start = current_end;
        }

        for (unsigned int t = 0; t < threads.size(); ++t) {
            if (threads[t].joinable()) {
                threads[t].join();
            }
        }
        std::cout << "Intersection calculation complete. Found " << all_intersections.size() << " intersections." << std::endl;
   }

    //需要对外访问
    static const int MAX_INTERSECTION_SIZE = 10;
    std::unique_ptr<WeightedGraph> pll_graph;                    // 专门为pll做的图，权值全保留
    std::vector<std::tuple<size_t, size_t, int>> all_intersections;

    std::vector<int> vertices;         // 顶点列表
    std::vector<Hyperedge> hyperedges; // 超边列表
    std::vector<std::vector<int>> vertex_to_edges;

    std::unique_ptr<DisjointSets> ds; // 使用智能指针存储预构建的并查集
    
    //重要，支持动态图更新
    bool ds_valid;                    // 新增：标记并查集是否有效

    // 新增：存储不同交集约束下的无向加权图

    std::vector<std::unique_ptr<WeightedGraph>> weighted_graphs; // 索引对应交集大小

    std::unique_ptr<WeightedPrunedLandmarkIndex> pll;            // 修改类型
    
    
    //重要，支持动态图更新
    bool graphs_built = false;                                   // 标记图是否已构建

    // --- 内存存储保持 bytes 单位 ---
    size_t ds_nodes = 0;
    size_t pll_total_label_size = 0;
    size_t weighted_graphs_total_nodes = 0;
    size_t weighted_graphs_total_edges = 0;

    size_t ds_memory_bytes = 0;
    std::vector<size_t> weighted_graphs_adj_list_memory_bytes; // Per layer
    std::vector<size_t> weighted_graphs_ds_memory_bytes;       // Per layer
    size_t pll_memory_bytes = 0;
    // --- 结束更新 ---
private:
};

#endif // HYPERGRAPH_H