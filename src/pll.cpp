#include "pll.h"
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <iostream>

// 构造函数，接收图结构
PLL::PLL(Graph& graph) : g(graph) {
    //做邻接表和逆邻接表
    buildAdjList(); 
    //做IN表和OUT表
    buildInOut();

}

void PLL::offline_industry()
{
    buildPLLLabels();
    adjList.clear();
    reverseAdjList.clear();
}

bool PLL::reachability_query(int source, int target)
{
    return query(source, target);
}



//构建邻接表和逆邻接表，遍历的时候用
void PLL::buildAdjList() {
    adjList.resize(g.vertices.size());
    reverseAdjList.resize(g.vertices.size());  // 初始化逆邻接表

    for (int node = 0; node < g.vertices.size(); ++node) {
        const auto& neighbors = g.vertices[node].LOUT;  // 使用LOUT构建邻接表
        for (int neighbor : neighbors) {
            adjList[node].push_back(neighbor);          // 邻接表
            reverseAdjList[neighbor].push_back(node);   // 逆邻接表，反向连接
        }
    }
}

void PLL::buildInOut()
{
    IN.resize(g.vertices.size());
    OUT.resize(g.vertices.size());
}

// 计算节点度，并按度降序排序节点
std::vector<int> PLL::orderByDegree() {
    // 初始化节点顺序数组
    std::vector<int> nodes;
    for (int i = 0; i < g.vertices.size(); ++i) {
        if (g.vertices[i].in_degree == 0  && g.vertices[i].out_degree==0) {
            continue;  // 跳过没有出度和入度的节点
        }
        nodes.push_back(i);
    }
    

    // 按照度的降序排序节点，度计算为 (in + 1) * (out + 1)
    std::sort(nodes.begin(), nodes.end(), [&](int a, int b) {
        int inDegreeA = g.vertices[a].LIN.empty() ? 0 : g.vertices[a].LIN.size();
        int outDegreeA = g.vertices[a].LOUT.empty() ? 0 : g.vertices[a].LOUT.size();
        int degreeA = (inDegreeA + 1) * (outDegreeA + 1);

        int inDegreeB = g.vertices[b].LIN.empty() ? 0 : g.vertices[b].LIN.size();
        int outDegreeB = g.vertices[b].LOUT.empty() ? 0 : g.vertices[b].LOUT.size();
        int degreeB = (inDegreeB + 1) * (outDegreeB + 1);

        return degreeA > degreeB;
    });

    return nodes;
}


void PLL::bfsPruned(int start){
    // 第一轮 BFS：从 start 出发，构建 IN 集合
    std::unordered_set<int> visited_forward;
    std::queue<int> q_forward;
    q_forward.push(start);
    visited_forward.insert(start);

    while (!q_forward.empty())
    {
        int current = q_forward.front();
        q_forward.pop();

        if (HopQuery(start, current)) continue;
        if(current != start) IN[current].push_back(start);

        for(auto neighbor : adjList[current]){
            if(!visited_forward.count(neighbor)) {
                q_forward.push(neighbor);
                visited_forward.insert(neighbor);
            }
        }
    }

    // 第二轮 BFS：从 start 出发，构建 OUT 集合
    std::unordered_set<int> visited_backward;
    std::queue<int> q_backward;
    q_backward.push(start);
    visited_backward.insert(start);

    while (!q_backward.empty())
    {
        int current = q_backward.front();
        q_backward.pop();

        if (HopQuery(current, start)) continue;
        if(current != start) OUT[current].push_back(start);

        for(auto neighbor : reverseAdjList[current]){
            if(!visited_backward.count(neighbor)) {
                q_backward.push(neighbor);
                visited_backward.insert(neighbor);
            }
        }
    }
}

void PLL::buildPLLLabels(){
    std::vector<int> nodes = orderByDegree();
    int i = 0;
    for (int node : nodes) {
        bfsPruned(node);
    }
    simplifyInOutSets();
}


// 检查是否存在2-hop路径，这样可以用来剪枝
// from u to v
bool PLL::HopQuery(int u, int v) {
    if (u >= g.vertices.size() || v >= g.vertices.size()) return false;
    const auto& LOUT_u = OUT[u];
    const auto& LIN_v = IN[v];

    std::unordered_set<int> loutSet(LOUT_u.begin(), LOUT_u.end());
    for (int node : LIN_v) {
        if (loutSet.count(node)) return true;  // 存在交集，表示可达
    }
    return false;  // 无交集，不可达
}

// 可达性查询，外部查询
bool PLL::query(int u, int v){

    if (u >= g.vertices.size() || v >= g.vertices.size()) return false;
    if (g.vertices[u].LOUT.empty() || g.vertices[v].LIN.empty()) return false;
    if (u == v) return true;
    
    for(auto out_u:OUT[u]){
        if(out_u == v) return true;
    }
    for(auto in_v:IN[v]){
        if(in_v == u) return true;
    }
    for(auto out_u:OUT[u]){
        for(auto in_v:IN[v]){
            if(out_u == in_v) return true;
        }
    }

    return false;  // 无交集，不可达
}


// 不剪枝的 BFS
void PLL::bfsUnpruned(int start, bool is_reversed) {
    std::queue<int> q;
    std::unordered_set<int> visited;  // 记录访问过的节点
    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        // 选择邻接方向
        const auto& neighbors = is_reversed ? g.vertices[current].LIN : g.vertices[current].LOUT;
        for (int neighbor : neighbors) {
            if (visited.find(neighbor) != visited.end()) {
                continue; // 如果已访问过，则跳过
            }

            // 更新标签：根据方向更新neighbor的in或out集合
            if (is_reversed) {
                g.vertices[neighbor].LOUT.push_back(start);  // 反向 BFS 更新 out 集合
            } else {
                g.vertices[neighbor].LIN.push_back(start);   // 正向 BFS 更新 in 集合
            }

            // 扩展到下一层
            visited.insert(neighbor);
            q.push(neighbor);
        }
    }
}

// 构建2-hop标签的PLL主函数（不剪枝版本）
void PLL::buildPLLLabelsUnpruned() {
    for (int node = 0; node < g.vertices.size(); ++node) {
        bfsUnpruned(node, false);  // 正向 BFS，构建LOUT
        bfsUnpruned(node, true);   // 反向 BFS，构建LIN
    }
}

//精简每个节点的in和out集合
void PLL::simplifyInOutSets() {
    for (auto& in_set : IN) {
        std::unordered_set<int> unique_in(in_set.begin(), in_set.end());
        in_set.assign(unique_in.begin(), unique_in.end());
    }

    for (auto& out_set : OUT) {
        std::unordered_set<int> unique_out(out_set.begin(), out_set.end());
        out_set.assign(unique_out.begin(), unique_out.end());
    }
}