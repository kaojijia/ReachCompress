#include "pll.h"
#include <queue>
#include <unordered_set>
#include <algorithm>  // 确保包含算法库

PLL::PLL(Graph& graph) : g(graph) {}

// 剪枝的BFS，用于构建2-hop索引
void PLL::bfsPruned(int start, bool is_reversed) {
    std::queue<int> q;
    std::unordered_set<int> visited;
    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        const auto& neighbors = is_reversed ? g.vertices[current].LIN : g.vertices[current].LOUT;
        for (int neighbor : neighbors) {
            if (visited.find(neighbor) != visited.end()) continue;

            // 剪枝条件：如果已经通过2-hop索引可达，跳过进一步遍历
            if (!is_reversed && std::find(g.vertices[start].LOUT.begin(), g.vertices[start].LOUT.end(), neighbor) != g.vertices[start].LOUT.end()) continue;
            if (is_reversed && std::find(g.vertices[start].LIN.begin(), g.vertices[start].LIN.end(), neighbor) != g.vertices[start].LIN.end()) continue;

            visited.insert(neighbor);

            if (is_reversed) {
                g.vertices[neighbor].LIN.push_back(start);
                q.push(neighbor);
            } else {
                g.vertices[neighbor].LOUT.push_back(start);
                q.push(neighbor);
            }
        }
    }
}

// 构建2-hop标签的PLL主函数
void PLL::buildPLLLabels() {
    for (int node = 0; node < g.vertices.size(); ++node) {
        bfsPruned(node, false);  // 正向BFS，构建LOUT
        bfsPruned(node, true);   // 反向BFS，构建LIN
    }
}

// 可达性查询
bool PLL::reachabilityQuery(int u, int v) {
    const auto& LOUT_u = g.vertices[u].LOUT;
    const auto& LIN_v = g.vertices[v].LIN;

    for (int node : LOUT_u) {
        if (std::find(LIN_v.begin(), LIN_v.end(), node) != LIN_v.end()) {
            return true;
        }
    }
    return false;
}

// 贪婪算法求解2-Hop覆盖
std::vector<int> PLL::greedy2HopCover() {
    std::vector<int> selectedNodes;
    std::unordered_set<int> coveredNodes;

    while (coveredNodes.size() < g.vertices.size()) {
        int bestNode = -1;
        int maxCover = 0;

        for (int node = 0; node < g.vertices.size(); ++node) {
            const auto& LOUT = g.vertices[node].LOUT;

            int uncoveredCount = 0;
            for (int neighbor : LOUT) {
                if (coveredNodes.find(neighbor) == coveredNodes.end()) {
                    uncoveredCount++;
                }
            }

            if (uncoveredCount > maxCover) {
                maxCover = uncoveredCount;
                bestNode = node;
            }
        }

        if (bestNode == -1) break;  // 没有更多节点可以覆盖

        selectedNodes.push_back(bestNode);
        for (int neighbor : g.vertices[bestNode].LOUT) {
            coveredNodes.insert(neighbor);
        }
    }

    return selectedNodes;
}