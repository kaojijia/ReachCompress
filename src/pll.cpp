#include "pll.h"
#include <queue>
#include <unordered_set>

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
            if (!is_reversed && g.vertices[start].LOUT.count(neighbor)) continue;
            if (is_reversed && g.vertices[start].LIN.count(neighbor)) continue;

            visited.insert(neighbor);

            if (is_reversed) {
                g.vertices[neighbor].LIN.insert(start);
                q.push(neighbor);
            } else {
                g.vertices[neighbor].LOUT.insert(start);
                q.push(neighbor);
            }
        }
    }
}

// 构建2-hop标签的PLL主函数
void PLL::buildPLLLabels() {
    for (auto& node_pair : g.vertices) {
        int node = node_pair.first;
        bfsPruned(node, false);  // 正向BFS，构建LOUT
        bfsPruned(node, true);   // 反向BFS，构建LIN
    }
}

// 可达性查询
bool PLL::reachabilityQuery(int u, int v) {
    const auto& LOUT_u = g.vertices[u].LOUT;
    const auto& LIN_v = g.vertices[v].LIN;

    for (int node : LOUT_u) {
        if (LIN_v.find(node) != LIN_v.end()) {
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

        for (const auto& pair : g.vertices) {
            int node = pair.first;
            const auto& LOUT = pair.second.LOUT;

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
