#ifndef HYPERGRAPH_H
#define HYPERGRAPH_H

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <memory>

#include "WeightedGraph.h"

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
        : ds_valid(false)
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

    // 添加顶点
    int addVertex()
    {
        ds_valid = false; // 数据变更，标记并查集失效
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
            if (v < 0 || v >= static_cast<int>(vertices.size()))
            {
                throw std::invalid_argument("Vertex ID in edge does not exist");
            }
            vertex_to_edges[v].push_back(edgeId);
        }
    }

    // 添加超边，所有参与顶点必须已存在
    int addHyperedge(const std::vector<int> &vertexList)
    {
        ds_valid = false; // 数据变更，标记并查集失效
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
    void offline_industry()
    {
        // 检查是否需要重建并查集
        if (!ds_valid || !ds)
        {
            // 重新创建并查集
            ds = std::make_unique<DisjointSets>(vertices.size());

            // 对每个超边，将其中的所有顶点合并到同一连通分量
            for (const auto &edge : hyperedges)
            {
                if (edge.vertices.empty())
                    continue;

                int first = edge.vertices[0];
                for (size_t i = 1; i < edge.vertices.size(); i++)
                {
                    ds->merge(first, edge.vertices[i]);
                }
            }

            ds_valid = true; // 标记并查集有效
        }

        // 构建反转后的无向加权图
        // 检查是否需要重建无向加权图
        if (!graphs_built)
        {
            // 清空现有的图
            weighted_graphs.clear();
            weighted_graphs.resize(MAX_INTERSECTION_SIZE + 1);

            // 预计算所有超边对的交集大小
            std::vector<std::tuple<int, int, int>> all_edge_pairs;
            all_edge_pairs.reserve(hyperedges.size() * hyperedges.size() / 2);

            for (size_t i = 0; i < hyperedges.size(); i++)
            {
                for (size_t j = i + 1; j < hyperedges.size(); j++)
                {
                    auto intersection = getHyperedgeIntersection(i, j);
                    if (!intersection.empty())
                    {
                        all_edge_pairs.emplace_back(i, j, intersection.size());
                    }
                }
            }

            // 构建每个交集约束对应的图
            for (int min_size = 0; min_size <= MAX_INTERSECTION_SIZE; min_size++)
            {
                // 创建对应约束级别的图
                weighted_graphs[min_size] = std::make_unique<WeightedGraph>(hyperedges.size(),min_size);

                // 只添加满足约束的边
                for (const auto &[i, j, size] : all_edge_pairs)
                {
                    if (size >= min_size)
                    {
                        weighted_graphs[min_size]->addEdge(i, j, size);
                    }
                }

                // 预处理该图的并查集，加速后续查询
                weighted_graphs[min_size]->offline_industry();
            }

            graphs_built = true;
        }
    }

    // 使用并查集构建连通分量
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
        const auto& source_edges = vertex_to_edges[source];
        const auto& target_edges = vertex_to_edges[target];
        
        for (int edge_id : source_edges) {
            if (std::find(target_edges.begin(), target_edges.end(), edge_id) != target_edges.end()) {
                return true;  // 两点属于同一超边，直接可达
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
    bool isReachableViaWeightedGraph(int sourceVertex, int targetVertex, int minIntersectionSize = 0)
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
                if (weighted_graphs[effective_min_size]->reachability_query(source_edge, target_edge))
                {
                    return true; // 找到一对可达的超边
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

private:
    std::vector<int> vertices;         // 顶点列表
    std::vector<Hyperedge> hyperedges; // 超边列表
    std::vector<std::vector<int>> vertex_to_edges;

    std::unique_ptr<DisjointSets> ds; // 使用智能指针存储预构建的并查集
    bool ds_valid;                    // 新增：标记并查集是否有效

    // 新增：存储不同交集约束下的无向加权图
    static const int MAX_INTERSECTION_SIZE = 10;
    std::vector<std::unique_ptr<WeightedGraph>> weighted_graphs; // 索引对应交集大小
    bool graphs_built = false;                                   // 标记图是否已构建
};

#endif // HYPERGRAPH_H