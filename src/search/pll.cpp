#include "pll.h"
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <iostream>

// 构造函数，接收图结构
PLL::PLL(Graph &graph) : g(graph)
{
    // 做邻接表和逆邻接表
    buildAdjList();
    // 做IN表和OUT表
    buildInOut();
}

void PLL::offline_industry()
{
    buildPLLLabels();
    adjList.clear();
    reverseAdjList.clear();
    // convertToArray();
    add_self();
}

bool PLL::reachability_query(int source, int target)
{
    return query(source, target);
    // 数组比vector的访问时间慢了不少，平均一个查询分别是5微秒和6微秒
    // return queryinArray(source, target);
}

bool PLL::queryinArray(int u, int v)
{
    uint32_t num_node = g.vertices.size();
    if (u > num_node || v > num_node)
        return false;
    if (u == v)
        return true;
    // 检查 IN 和 OUT 集合
    for (int i = in_pointers[v]; i < in_pointers[v + 1]; ++i)
    {
        if (in_sets[i] == u)
            return true;
    }
    for (int i = out_pointers[u]; i < out_pointers[u + 1]; ++i)
    {
        if (out_sets[i] == v)
            return true;
    }
    for (int i = out_pointers[u]; i < out_pointers[u + 1]; ++i)
    {
        for (int j = in_pointers[v]; j < in_pointers[v + 1]; ++j)
        {
            if (out_sets[i] == in_sets[j])
                return true;
        }
    }

    return false; // 无交集，不可达
}

// 构建邻接表和逆邻接表，遍历的时候用
void PLL::buildAdjList()
{
    adjList.resize(g.vertices.size());
    reverseAdjList.resize(g.vertices.size()); // 初始化逆邻接表

    for (int node = 0; node < g.vertices.size(); ++node)
    {
        const auto &neighbors = g.vertices[node].LOUT; // 使用LOUT构建邻接表
        for (int neighbor : neighbors)
        {
            adjList[node].push_back(neighbor);        // 邻接表
            reverseAdjList[neighbor].push_back(node); // 逆邻接表，反向连接
        }
    }
}

void PLL::buildInOut()
{
    IN.resize(g.vertices.size());
    OUT.resize(g.vertices.size());
}

// 计算节点度，并按度降序排序节点
std::vector<int> PLL::orderByDegree()
{
    // 初始化节点顺序数组
    std::vector<int> nodes;
    for (int i = 0; i < g.vertices.size(); ++i)
    {
        if (g.vertices[i].in_degree == 0 && g.vertices[i].out_degree == 0)
        {
            continue; // 跳过没有出度和入度的节点
        }
        nodes.push_back(i);
    }

    // 按照度的降序排序节点，度计算为 (in + 1) * (out + 1)
    std::sort(nodes.begin(), nodes.end(), [&](int a, int b)
              {
        int inDegreeA = g.vertices[a].LIN.empty() ? 0 : g.vertices[a].LIN.size();
        int outDegreeA = g.vertices[a].LOUT.empty() ? 0 : g.vertices[a].LOUT.size();
        int degreeA = (inDegreeA + 1) * (outDegreeA + 1);

        int inDegreeB = g.vertices[b].LIN.empty() ? 0 : g.vertices[b].LIN.size();
        int outDegreeB = g.vertices[b].LOUT.empty() ? 0 : g.vertices[b].LOUT.size();
        int degreeB = (inDegreeB + 1) * (outDegreeB + 1);

        return degreeA > degreeB; });

    return nodes;
}

bool PLL::convertToArray()
{
    // 分配内存
    int max_node_id = g.vertices.size();
    in_pointers = new uint32_t[max_node_id + 1];
    out_pointers = new uint32_t[max_node_id + 1];

    size_t in_total_size = 0;
    size_t out_total_size = 0;

    for (const auto &in_set : IN)
    {
        in_total_size += in_set.size();
    }
    for (const auto &out_set : OUT)
    {
        out_total_size += out_set.size();
    }

    in_sets = new uint32_t[in_total_size];
    out_sets = new uint32_t[out_total_size];

    // 填充 in_pointers 和 in_sets
    size_t in_index = 0;
    in_pointers[0] = 0;
    for (size_t i = 0; i < IN.size(); ++i)
    {
        in_pointers[i + 1] = in_pointers[i] + IN[i].size();
        for (auto j : IN[i])
        {
            in_sets[in_index++] = j;
        }
    }

    // 填充 out_pointers 和 out_sets
    size_t out_index = 0;
    out_pointers[0] = 0;
    for (size_t i = 0; i < OUT.size(); ++i)
    {
        out_pointers[i + 1] = out_pointers[i] + OUT[i].size();
        for (auto j : OUT[i])
        {
            out_sets[out_index++] = j;
        }
    }

    this->in_sets_length = in_total_size;
    this->out_sets_length = out_total_size;
    this->pointer_length = max_node_id + 1;
    return true;
}


void PLL::bfsPruned(int start)
{
    // 使用 vector<bool> 来记录正向 BFS 中的访问情况，避免 unordered_set 的哈希开销
    std::vector<bool> visited_forward(g.vertices.size(), false);
    std::queue<int> q_forward;
    q_forward.push(start);
    visited_forward[start] = true;  // 标记起点已访问

    // 第一轮 BFS：从 start 出发，构建 IN 集合
    while (!q_forward.empty())
    {
        int current = q_forward.front();
        q_forward.pop();

        // 如果已有标签证明 start 到 current 可达，则剪枝
        if (HopQuery(start, current))
            continue;
        // 除了起点外，将 start 加入 current 的 IN 标签集合中
        if (current != start)
            insertSorted(IN[current], start);
            // IN[current].insert(start);

        // 遍历 current 的后继节点
        for (auto neighbor : adjList[current])
        {
            if (!visited_forward[neighbor])
            {
                q_forward.push(neighbor);
                visited_forward[neighbor] = true;
            }
        }
    }

    // 使用 vector<bool> 来记录反向 BFS 的访问情况
    std::vector<bool> visited_backward(g.vertices.size(), false);
    std::queue<int> q_backward;
    q_backward.push(start);
    visited_backward[start] = true;  // 标记起点已访问

    // 第二轮 BFS：从 start 出发，构建 OUT 集合
    while (!q_backward.empty())
    {
        int current = q_backward.front();
        q_backward.pop();

        // 如果已有标签证明 current 到 start 可达，则剪枝
        if (HopQuery(current, start))
            continue;
        // 除了起点外，将 start 加入 current 的 OUT 标签集合中
        if (current != start)
            insertSorted(OUT[current], start);
            // OUT[current].insert(start);

        // 遍历 current 的前驱节点（反向邻接表）
        for (auto neighbor : reverseAdjList[current])
        {
            if (!visited_backward[neighbor])
            {
                q_backward.push(neighbor);
                visited_backward[neighbor] = true;
            }
        }
    }
}



// void PLL::bfsPruned(int start)
// {
//     // 第一轮 BFS：从 start 出发，构建 IN 集合
//     std::unordered_set<int> visited_forward;
//     std::queue<int> q_forward;
//     q_forward.push(start);
//     visited_forward.insert(start);

//     while (!q_forward.empty())
//     {
//         int current = q_forward.front();
//         q_forward.pop();

//         if (HopQuery(start, current))
//             continue;
//         if (current != start)
//             IN[current].insert(start);

//         for (auto neighbor : adjList[current])
//         {
//             if (!visited_forward.count(neighbor))
//             {
//                 q_forward.push(neighbor);
//                 visited_forward.insert(neighbor);
//             }
//         }
//     }

//     // 第二轮 BFS：从 start 出发，构建 OUT 集合
//     std::unordered_set<int> visited_backward;
//     std::queue<int> q_backward;
//     q_backward.push(start);
//     visited_backward.insert(start);

//     while (!q_backward.empty())
//     {
//         int current = q_backward.front();
//         q_backward.pop();

//         if (HopQuery(current, start))
//             continue;
//         if (current != start)
//             OUT[current].insert(start);

//         for (auto neighbor : reverseAdjList[current])
//         {
//             if (!visited_backward.count(neighbor))
//             {
//                 q_backward.push(neighbor);
//                 visited_backward.insert(neighbor);
//             }
//         }
//     }
// }


// 检查是否存在2-hop路径，这样可以用来剪枝
// from u to v
bool PLL::HopQuery(int u, int v)
{
    if (u >= g.vertices.size() || v >= g.vertices.size())
        return false;
    const auto &LOUT_u = OUT[u]; // set<int>
    const auto &LIN_v = IN[v];   // set<int>

    auto it1 = LOUT_u.begin();
    auto it2 = LIN_v.begin();
    while (it1 != LOUT_u.end() && it2 != LIN_v.end())
    {
        if (*it1 == *it2)
        {
            return true;
        }
        else if (*it1 < *it2)
        {
            ++it1;
        }
        else
        {
            ++it2;
        }
    }
    return false;
}


// bool PLL::HopQuery(int u, int v) {
//     if (u >= g.vertices.size() || v >= g.vertices.size()) return false;
//     const auto& LOUT_u = OUT[u];
//     const auto& LIN_v = IN[v];

//     std::unordered_set<int> loutSet(LOUT_u.begin(), LOUT_u.end());
//     for (int node : LIN_v) {
//         if (loutSet.count(node)) return true;  // 存在交集，表示可达
//     }
//     return false;  // 无交集，不可达
// }

void PLL::add_self()
{
    for(int i = 0; i < g.vertices.size(); i++){
        // IN[i].insert(i);
        // OUT[i].insert(i);
        insertSorted(IN[i], i);
        insertSorted(OUT[i], i);

    }
}

void PLL::buildPLLLabels()
{
    std::vector<int> nodes = orderByDegree();
    int i = 0;
    int count = 0;
    for (int node : nodes)
    {
        bfsPruned(node);
#ifdef DEBUG
        if (++count % 10000 == 0)
        {
            std::cout << Algorithm::getCurrentTimestamp() << "PLL处理完成 " << count << " 个节点的索引..." << std::endl;
        }
#endif
    }


    simplifyInOutSets();
}


// 可达性查询，外部查询
bool PLL::query(int u, int v)
{

    if (u >= g.vertices.size() || v >= g.vertices.size())
        return false;
    if (g.vertices[u].LOUT.empty() || g.vertices[v].LIN.empty())
        return false;
    if (u == v)
        return true;


    for (auto out_u : OUT[u])
    {
        for (auto in_v : IN[v])
        {
            if (out_u == in_v)
                return true;
        }
    }


    return false; // 无交集，不可达
}

// 不剪枝的 BFS
void PLL::bfsUnpruned(int start, bool is_reversed)
{
    std::queue<int> q;
    std::unordered_set<int> visited; // 记录访问过的节点
    q.push(start);
    visited.insert(start);

    while (!q.empty())
    {
        int current = q.front();
        q.pop();

        // 选择邻接方向
        const auto &neighbors = is_reversed ? g.vertices[current].LIN : g.vertices[current].LOUT;
        for (int neighbor : neighbors)
        {
            if (visited.find(neighbor) != visited.end())
            {
                continue; // 如果已访问过，则跳过
            }

            // 更新标签：根据方向更新neighbor的in或out集合
            if (is_reversed)
            {
                g.vertices[neighbor].LOUT.push_back(start); // 反向 BFS 更新 out 集合
            }
            else
            {
                g.vertices[neighbor].LIN.push_back(start); // 正向 BFS 更新 in 集合
            }

            // 扩展到下一层
            visited.insert(neighbor);
            q.push(neighbor);
        }
    }
}

// 构建2-hop标签的PLL主函数（不剪枝版本）
void PLL::buildPLLLabelsUnpruned()
{
    for (int node = 0; node < g.vertices.size(); ++node)
    {
        bfsUnpruned(node, false); // 正向 BFS，构建LOUT
        bfsUnpruned(node, true);  // 反向 BFS，构建LIN
    }
}

// 精简每个节点的in和out集合
//  void PLL::simplifyInOutSets() {
//      for (auto& in_set : IN) {
//          std::unordered_set<int> unique_in(in_set.begin(), in_set.end());
//          in_set.assign(unique_in.begin(), unique_in.end());
//      }

//     for (auto& out_set : OUT) {
//         std::unordered_set<int> unique_out(out_set.begin(), out_set.end());
//         out_set.assign(unique_out.begin(), unique_out.end());
//     }
// }

void PLL::simplifyInOutSets()
{
    // for (auto& in_set : IN) {
    //     std::sort(in_set.begin(), in_set.end());
    //     auto last = std::unique(in_set.begin(), in_set.end());
    //     in_set.erase(last, in_set.end());
    // }

    // for (auto& out_set : OUT) {
    //     std::sort(out_set.begin(), out_set.end());
    //     auto last = std::unique(out_set.begin(), out_set.end());
    //     out_set.erase(last, out_set.end());
    // }
}
PLL::~PLL()
{
    // 释放动态数组
    // if (in_pointers != nullptr) {
    //     delete[] in_pointers;
    //     in_pointers = nullptr;
    // }
    // if (out_pointers != nullptr) {
    //     delete[] out_pointers;
    //     out_pointers = nullptr;
    // }
    // if (in_sets != nullptr) {
    //     delete[] in_sets;
    //     in_sets = nullptr;
    // }
    // if (out_sets != nullptr) {
    //     delete[] out_sets;
    //     out_sets = nullptr;
    // }

    // 清空 IN 和 OUT 容器
    if (!IN.empty())
    {
        // std::cout << "Clearing IN..." << std::endl;
        for (auto &inSet : IN)
        {
            inSet.clear(); // 清空每个子向量
        }
        IN.clear(); // 清空主向量
    }

    if (!OUT.empty())
    {
        // std::cout << "Clearing OUT..." << std::endl;
        for (auto &outSet : OUT)
        {
            outSet.clear();
        }
        OUT.clear();
    }

    // 清空邻接表
    if (!adjList.empty())
    {
        // std::cout << "Clearing adjList..." << std::endl;
        adjList.clear();
    }
    if (!reverseAdjList.empty())
    {
        // std::cout << "Clearing reverseAdjList..." << std::endl;
        reverseAdjList.clear();
    }

    // std::cout << "PLL destructor completed." << std::endl;
}



// 有序插入：
void PLL::insertSorted(std::vector<int>& labelVec, int val)
{
    // 利用二分查找找到应插入的位置
    auto it = std::lower_bound(labelVec.begin(), labelVec.end(), val);
    // 如果不存在该元素，则插入
    if (it == labelVec.end() || *it != val) {
        labelVec.insert(it, val);
    }
}
