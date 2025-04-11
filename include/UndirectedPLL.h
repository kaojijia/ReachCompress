#pragma once

#include"WeightedGraph.h"
#include<iostream>
#include <vector>
#include <limits>
#include <stdexcept>
#include <memory>
#include <queue>
#include <utility>
#include <algorithm>


using namespace std;


class PrunedLandmarkIndex
{
public:
    // 构造函数：需要引用无向加权图
    PrunedLandmarkIndex(const WeightedGraph &graph);

    // 建立离线索引
    // threshold: 边权小于 threshold 的边将被忽略，视为不存在
    void offline_industry(int threshold);

    // 查询可达性：判断两点是否在忽略小权重边的情况下连通
    bool reachability_query(int u, int v) const;

private:
    // 内部存储的图引用
    const WeightedGraph &g;

    // label[v]：存储所有能使v和其他点相通的Landmark
    std::vector<std::vector<int>> label;

    int weightThreshold; // 在buildIndex时设定

    // 对顶点做 BFS，更新标签
    // curLandmark: 当前使用的Landmark
    void prunedBFS(int curLandmark);

    // 判断是否可以通过已有标签推断 curLandmark 与 node 可达
    bool hopQuery(int curLandmark, int node) const;

    // 辅助函数：在有序的 label[node] 中插入 val（若不存在则插入）
    void insertSorted(std::vector<int> &vec, int val) const;

    // 判断两个有序 vector 是否有交集
    bool intersect(const std::vector<int> &vec1,
                   const std::vector<int> &vec2) const;
};

// ============ 构造函数 ============
PrunedLandmarkIndex::PrunedLandmarkIndex(const WeightedGraph &graph)
    : g(graph)
{
    // 先把 label 容器 resize 到图的顶点数
    label.resize(g.numVertices());
}

// ============ 对外的 buildIndex ============
void PrunedLandmarkIndex::offline_industry(int threshold)
{
    // 保存边权阈值，后续 BFS 中忽略权重大于此阈值的边
    this->weightThreshold = threshold;

    int n = g.numVertices();
    // 计算每个顶点的度（邻接表大小）
    std::vector<std::pair<int, int>> degrees(n);
    for (int i = 0; i < n; ++i)
    {
        // 这里度定义为邻接边数；g.getNeighbors(i) 返回引用，
        // 如果需要可以用其他度策略（例如加权度）替换
        degrees[i] = {static_cast<int>(g.getNeighbors(i).size()), i};
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
        insertSorted(label[i], i);
    }
}
// ============ prunedBFS 实现：只做无向 BFS，忽略小权重边 ============
void PrunedLandmarkIndex::prunedBFS(int curLandmark)
{
    // 准备一个 visited 数组，防止重复遍历
    std::vector<bool> visited(g.numVertices(), false);
    visited[curLandmark] = true;

    // BFS 队列
    std::queue<int> q;
    q.push(curLandmark);

    while (!q.empty())
    {
        int current = q.front();
        q.pop();

        // 若已有标签可推断 curLandmark->current 可达，则剪枝
        // （对起点自己可以写 if (current != curLandmark) 之类，但也可一并覆盖）
        if (hopQuery(curLandmark, current) && current != curLandmark)
        {
            continue;
        }

        // 否则，把 curLandmark 这个标签加入 current 的 label
        if (current != curLandmark)
        {
            insertSorted(label[current], curLandmark);
        }

        // 扩展 current 的邻居
        // 忽略 weight < threshold 的边
        const auto &neighbors = g.getNeighbors(current);
        for (auto &[nbr, w] : neighbors)
        {
            if (w >= weightThreshold && !visited[nbr])
            {
                visited[nbr] = true;
                q.push(nbr);
            }
        }
    }
}

// ============ hopQuery ============
bool PrunedLandmarkIndex::hopQuery(int curLandmark, int node) const
{
    // 核心就是看 label[curLandmark] 与 label[node] 是否有交集
    // 不过这里注释提到：对于无向图，我们不需要 label[curLandmark]？
    // 传统 PLL 在有向图中看 L_OUT[u], L_IN[v] 交集;
    // 此处为简化, 只需看 label[curLandmark] 与 label[node] 是否有公共 Landmark
    // 但 curLandmark 本身没有 label，它作为 Landmark 的"identity"。
    // => 其实我们更常用的是: 只需检查 label[node] 是否包含 curLandmark，
    //    或者 label[node] 与 label[curLandmark本身的标签]交集?
    // 这里给出一个更通用写法：看 "label[curLandmark]" 与 "label[node]" 是否交集。

    // 先特殊处理：如果 node == curLandmark 则可直接返回true
    if (node == curLandmark)
    {
        return true;
    }

    // 如果 curLandmark 没在 label[curLandmark] 里, 也行
    // 这里当curLandmark是"landmark节点", label[curLandmark]可能是空或很小.
    // 直接做向量交集： label[curLandmark], label[node]
    // 让 intersect 函数统一处理
    return intersect(label[curLandmark], label[node]);
}

// ============ 插入 val 到 vec 中（有序，不重复） ============
void PrunedLandmarkIndex::insertSorted(std::vector<int> &vec, int val) const
{
    auto it = std::lower_bound(vec.begin(), vec.end(), val);
    if (it == vec.end() || *it != val)
    {
        vec.insert(it, val);
    }
}

// ============ 有序vector是否有交集 ============
bool PrunedLandmarkIndex::intersect(const std::vector<int> &vec1,
                                    const std::vector<int> &vec2) const
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

// ============ 对外查询 ============
// 若 label[u] 与 label[v] 有交集，则表示可达
bool PrunedLandmarkIndex::reachability_query(int u, int v) const
{
    if (u < 0 || u >= (int)g.numVertices() ||
        v < 0 || v >= (int)g.numVertices())
        throw std::invalid_argument("Vertex does not exist");

    if (u == v)
        return true;

    // 直接判断 label[u] & label[v] 是否交集
    return intersect(label[u], label[v]);
}