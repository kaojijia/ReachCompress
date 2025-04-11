#pragma once
#include "WeightedGraph.h"
#include <vector>
#include <queue>
#include <algorithm>

/*
 * WeightedPrunedLandmarkIndex 实现了基于 2-hop 索引的无向加权图可达性（LCR）查询。
 *
 * 设计说明：
 * 1. 对于每个节点 v，我们维护 label[v]（一个有序 vector），其中存储 (landmark, bottleneck) 对，
 *    表示从 landmark 到 v 的路径上各边最小的权值（即瓶颈）。
 *
 * 2. offline_industry() 函数：
 *    - 首先对所有顶点按其邻接边数（度）降序排序，依次将每个顶点作为 landmark 进行 BFS 扩展；
 *    - 扩展过程中使用类似 Dijkstra 的 BFS，从 landmark 出发，
 *      每个状态记录 (node, current_bottleneck)，当前瓶颈值即为路径上所有边中的最小值；
 *    - 在扩展到某节点时调用 hopQuery(curLandmark, node, candidateBW) 判断，
 *      若 label[curLandmark] 与 label[node] 中已经存在“足够好”的记录（即瓶颈 ≥ candidateBW），
 *      则跳过扩展；否则将 (curLandmark, candidateBW) 记录插入 label[node] 。
 *    - 最后将每个节点自身（(v, INT_MAX)）加入 label[v]。
 *
 * 3. reachability_query(u, v, queryThreshold) 接口：
 *    - 查询时给定查询阈值 queryThreshold，仅当 u 与 v 的 label 中存在公共 landmark，
 *      且两个记录的瓶颈值的较小值 ≥ queryThreshold 时，返回 true，否则返回 false。
 *
 * 注意：离线构建时不进行边权的过滤（即所有边均参与扩展）。
 */

class WeightedPrunedLandmarkIndex {
public:
    // 构造函数：传入 WeightedGraph 引用；weightThreshold 参数仅为内部剪枝时（初始可设为 0）
    WeightedPrunedLandmarkIndex(const WeightedGraph &graph, int weightThreshold = 0)
        : g(graph), weightThreshold(weightThreshold)
    {
        label.resize(g.numVertices());
    }

    // 离线构建索引，函数名为 offline_industry()
    void offline_industry();

    // 查询可达性，函数名为 reachability_query(u, v, queryThreshold)
    // 如果在查询时 u 与 v 存在公共 landmark，其记录的瓶颈值（取二者较小值） ≥ queryThreshold，则返回 true
    bool reachability_query(int u, int v, int queryThreshold) const;

private:
    const WeightedGraph &g;
    int weightThreshold; // 内部剪枝时可用，通常设为 0 表示不额外过滤
    // label[v]: 存储 (landmark, bottleneck) 对，按 landmark 升序排列
    std::vector<std::vector<std::pair<int, int>>> label;

    // 在有序 vector 中插入或更新 (landmark, bw) 记录
    void insertOrUpdateLabel(std::vector<std::pair<int, int>> &vec, int landmark, int bw) const;

    // 在剪枝判断中，判断节点 node 是否已由 Landmark curLandmark 到达，
    // 并且已记录的路径瓶颈值 ≥ candidateBW
    bool hopQuery(int curLandmark, int node, int candidateBW) const;

    // prunedBFS：以 curLandmark 为起点构造索引，采用 BFS 传播真实边权信息
    // 状态为 (node, current_bottleneck)，当前瓶颈为路径上所有边的最小权值。
    void prunedBFS(int curLandmark);

    // 辅助函数：判断两个有序 vector（记录 (landmark, bottleneck)）是否有公共记录，
    // 并且对于公共 landmark，其瓶颈值的较小值 ≥ threshold。
    bool intersectWithThreshold(const std::vector<std::pair<int, int>> &a,
                                  const std::vector<std::pair<int, int>> &b,
                                  int threshold) const;
};



// ----------------------- 实现部分 -------------------------

void WeightedPrunedLandmarkIndex::offline_industry() {
    int n = g.numVertices();
    // 计算每个顶点的度（邻接边数），并按降序排序
    std::vector<std::pair<int, int>> deg(n);
    for (int i = 0; i < n; ++i) {
        deg[i] = { static_cast<int>(g.getNeighbors(i).size()), i };
    }
    std::sort(deg.begin(), deg.end(), [](const auto &a, const auto &b) {
        return a.first > b.first;
    });
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) {
        order[i] = deg[i].second;
    }
    // 按降序顺序依次将每个顶点作为 Landmark 进行 BFS 扩展
    for (int landmark : order) {
        prunedBFS(landmark);
    }
    // 最后，将每个顶点加入自身标签，保证自身可达性
    for (int i = 0; i < n; ++i) {
        insertOrUpdateLabel(label[i], i, std::numeric_limits<int>::max());
    }
}

void WeightedPrunedLandmarkIndex::insertOrUpdateLabel(std::vector<std::pair<int, int>> &vec,
                                                       int landmark, int bw) const {
    auto it = std::lower_bound(vec.begin(), vec.end(), landmark,
        [](const std::pair<int, int> &p, int val) {
            return p.first < val;
        });
    if (it != vec.end() && it->first == landmark) {
        // 更新为更高的瓶颈值
        if (bw > it->second) {
            it->second = bw;
        }
    } else {
        vec.insert(it, { landmark, bw });
    }
}

bool WeightedPrunedLandmarkIndex::hopQuery(int curLandmark, int node, int candidateBW) const {
    // 如果 node == curLandmark 则直接认为可达（自身）
    if (node == curLandmark) return true;

    const auto &vec1 = label[curLandmark];
    const auto &vec2 = label[node];
    int i = 0, j = 0;
    while(i < (int)vec1.size() && j < (int)vec2.size()){
        if(vec1[i].first == vec2[j].first){
            int bw = std::min(vec1[i].second, vec2[j].second);
            if(bw >= candidateBW)
                return true;
            ++i;
            ++j;
        } else if(vec1[i].first < vec2[j].first){
            ++i;
        } else {
            ++j;
        }
    }
    return false;
}

bool WeightedPrunedLandmarkIndex::intersectWithThreshold(
    const std::vector<std::pair<int, int>> &a,
    const std::vector<std::pair<int, int>> &b,
    int threshold) const {
    int i = 0, j = 0;
    while(i < (int)a.size() && j < (int)b.size()){
        if(a[i].first == b[j].first){
            int bw = std::min(a[i].second, b[j].second);
            if(bw >= threshold)
                return true;
            ++i; ++j;
        } else if(a[i].first < b[j].first){
            ++i;
        } else {
            ++j;
        }
    }
    return false;
}

void WeightedPrunedLandmarkIndex::prunedBFS(int curLandmark) {
    int n = g.numVertices();
    // 初始化 visited 数组，防止重复遍历
    std::vector<bool> visited(n, false);
    // 队列中存储 (node, current_bottleneck)，初始状态从 curLandmark 开始，瓶颈为无限大
    std::queue<std::pair<int, int>> q;
    visited[curLandmark] = true;
    q.push({ curLandmark, std::numeric_limits<int>::max() });
    
    while(!q.empty()){
        auto [current, bw] = q.front();
        q.pop();
        // 剪枝：如果当前节点（非 curLandmark）在 label 中已存在 curLandmark 记录
        // 且记录的瓶颈值 ≥ 当前候选 bw，则跳过扩展
        if(current != curLandmark && hopQuery(curLandmark, current, bw))
            continue;
        if(current != curLandmark)
            insertOrUpdateLabel(label[current], curLandmark, bw);
        
        // 扩展 current 的所有邻居，注意：此处不过滤任何边
        for(const auto &edge : g.getNeighbors(current)){
            int v = edge.first;
            int w = edge.second;
            if(!visited[v]){
                visited[v] = true;
                int new_bw = std::min(bw, w);
                q.push({v, new_bw});
            }
        }
    }
}

bool WeightedPrunedLandmarkIndex::reachability_query(int u, int v, int queryThreshold) const {
    if(u < 0 || u >= (int)g.numVertices() || v < 0 || v >= (int)g.numVertices())
        throw std::invalid_argument("Vertex id out of range");
    if(u == v) return true;
    
    // 查询时：若 label[u] 与 label[v] 存在公共 landmark
    // 且二者记录的瓶颈值的较小值 ≥ queryThreshold，则认为 u 和 v 可达
    return intersectWithThreshold(label[u], label[v], queryThreshold);
}

