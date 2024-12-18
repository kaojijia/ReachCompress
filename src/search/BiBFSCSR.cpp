#include "BiBFSCSR.h"
#include "graph.h"
#include "CSR.h"

BiBFSCSR::BiBFSCSR(Graph &graph) : g(graph) {
    csr = new CSRGraph();
    csr->fromGraph(g);
}

bool BiBFSCSR::reachability_query(int source, int target)
{
    auto result = findPath(source, target);
    if (!result.empty()) {
        return true;
    }
    return false;
}

std::vector<int> BiBFSCSR::findPath(int source, int target, int partition_number)
{
    if (source == target) {
        return {source};
    }

    std::queue<int> queue_forward, queue_backward;
    std::unordered_set<int> visited_forward, visited_backward;
    std::unordered_map<int, int> parent_forward, parent_backward;

    queue_forward.push(source);
    visited_forward.insert(source);
    parent_forward[source] = -1;

    queue_backward.push(target);
    visited_backward.insert(target);
    parent_backward[target] = -1;

    while (!queue_forward.empty() && !queue_backward.empty()) {
        // 正向搜索
        if (!queue_forward.empty()) {
            int current = queue_forward.front();
            queue_forward.pop();

            uint32_t degree;
            uint32_t* neighbors = csr->getOutgoingEdges(current, degree);
            if (neighbors != nullptr) {
                for (uint32_t i = 0; i < degree; ++i) {
                    int neighbor = neighbors[i];
                    if (visited_backward.find(neighbor) != visited_backward.end()) {
                        // 找到路径
                        std::vector<int> path;
                        int node = current;
                        while (node != -1) {
                            path.push_back(node);
                            node = parent_forward[node];
                        }
                        std::reverse(path.begin(), path.end());
                        node = neighbor;
                        while (node != -1) {
                            path.push_back(node);
                            node = parent_backward[node];
                        }
                        return path;
                    }
                    if (visited_forward.find(neighbor) == visited_forward.end()) {
                        queue_forward.push(neighbor);
                        visited_forward.insert(neighbor);
                        parent_forward[neighbor] = current;
                    }
                }
            }
        }

        // 反向搜索
        if (!queue_backward.empty()) {
            int current = queue_backward.front();
            queue_backward.pop();

            uint32_t degree;
            uint32_t* neighbors = csr->getIncomingEdges(current, degree);
            if (neighbors != nullptr) {
                for (uint32_t i = 0; i < degree; ++i) {
                    int neighbor = neighbors[i];
                    if (visited_forward.find(neighbor) != visited_forward.end()) {
                        // 找到路径
                        std::vector<int> path;
                        int node = neighbor;
                        while (node != -1) {
                            path.push_back(node);
                            node = parent_forward[node];
                        }
                        std::reverse(path.begin(), path.end());
                        node = current;
                        while (node != -1) {
                            path.push_back(node);
                            node = parent_backward[node];
                        }
                        return path;
                    }
                    if (visited_backward.find(neighbor) == visited_backward.end()) {
                        queue_backward.push(neighbor);
                        visited_backward.insert(neighbor);
                        parent_backward[neighbor] = current;
                    }
                }
            }
        }
    }

    // 未找到路径
    return {};
}