#include "compression.h"
#include <algorithm>
#include <stack>
#include <unordered_set>

Compression::Compression(Graph& graph) : g(graph) {
    inNout1.clear();
    in1outN.clear();
    in1out1.clear();
}

// 检查节点是否已合并
bool Compression::isMerged(int node) const {
    return mergedNodes.find(node) != mergedNodes.end();
}

// 标记节点已合并
void Compression::markAsMerged(int node) {
    mergedNodes.insert(node);
}

// 压缩有向无环子图
void Compression::compressRAG() {
    std::vector<int> topo_order = topologicalSort();
    std::unordered_set<int> rag_nodes(topo_order.begin(), topo_order.end());

    int new_node = g.vertices.size();
    g.vertices.push_back(Vertex());

    for (int node : topo_order) {
        for (int neighbor : g.vertices[node].LOUT) {
            if (rag_nodes.find(neighbor) == rag_nodes.end()) {
                g.vertices[new_node].LOUT.push_back(neighbor);
                g.vertices[neighbor].LIN.push_back(new_node);
            }
        }
        for (int neighbor : g.vertices[node].LIN) {
            if (rag_nodes.find(neighbor) == rag_nodes.end()) {
                g.vertices[new_node].LIN.push_back(neighbor);
                g.vertices[neighbor].LOUT.push_back(new_node);
            }
        }
    }

    for (int node : topo_order) {
        markAsMerged(node);
    }

    for (int node : topo_order) {
        if (isMerged(node)) {
            removeNode(node);
        }
    }
}

// 删除节点
void Compression::removeNode(int node) {
    for (int neighbor : g.vertices[node].LOUT) {
        removeEdgeFromNode(node, neighbor);
    }
    for (int neighbor : g.vertices[node].LIN) {
        removeEdgeFromNode(neighbor, node);
    }
    g.vertices[node].LOUT.clear();
    g.vertices[node].LIN.clear();
}

// 合并节点
void Compression::mergeNodes(int u, int v) {
    if (std::find(g.vertices[u].LOUT.begin(), g.vertices[u].LOUT.end(), v) != g.vertices[u].LOUT.end()) {
        for (int neighbor : g.vertices[v].LOUT) {
            if (std::find(g.vertices[u].LOUT.begin(), g.vertices[u].LOUT.end(), neighbor) == g.vertices[u].LOUT.end()) {
                g.vertices[u].LOUT.push_back(neighbor);
                g.vertices[neighbor].LIN.push_back(u);
            }
        }
    }

    if (std::find(g.vertices[u].LIN.begin(), g.vertices[u].LIN.end(), v) != g.vertices[u].LIN.end()) {
        for (int neighbor : g.vertices[v].LIN) {
            if (std::find(g.vertices[u].LIN.begin(), g.vertices[u].LIN.end(), neighbor) == g.vertices[u].LIN.end()) {
                g.vertices[u].LIN.push_back(neighbor);
                g.vertices[neighbor].LOUT.push_back(u);
            }
        }
    }

    mapping[u].push_back(v);
    markAsMerged(v);
}

// 删除边
void Compression::removeEdge(int u, int v, bool is_directed) {
    removeEdgeFromNode(u, v);

    if (!is_directed) {
        removeEdgeFromNode(v, u);
    }
}

// 从节点 u 中删除边 v
void Compression::removeEdgeFromNode(int u, int v) {
    auto& lout_neighbors = g.vertices[u].LOUT;
    lout_neighbors.erase(std::remove(lout_neighbors.begin(), lout_neighbors.end(), v), lout_neighbors.end());
    g.vertices[u].out_degree--;

    auto& lin_neighbors = g.vertices[v].LIN;
    lin_neighbors.erase(std::remove(lin_neighbors.begin(), lin_neighbors.end(), u), lin_neighbors.end());
    g.vertices[v].in_degree--;
}

// 核心压缩方法
void Compression::compress(int u, int v) {
    mapping[u].push_back(v);
    mergeNodes(u, v);
}

// 拓扑排序
std::vector<int> Compression::topologicalSort() {
    std::vector<int> result;
    std::vector<bool> visited(g.vertices.size(), false);

    for (int i = 0; i < g.vertices.size(); ++i) {
        if (!visited[i]) {
            dfs(i, visited, result);
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

// 深度优先搜索
void Compression::dfs(int node, std::vector<bool>& visited, std::vector<int>& result) {
    visited[node] = true;

    for (int neighbor : g.vertices[node].LOUT) {
        if (!visited[neighbor]) {
            dfs(neighbor, visited, result);
        }
    }

    result.push_back(node);
}

// 归类节点
void Compression::classifyVertex(int vertex) {
    int in_degree = g.vertices[vertex].LIN.size();
    int out_degree = g.vertices[vertex].LOUT.size();

    if (out_degree == 1 && in_degree > 1) {
        inNout1.insert(vertex);
    } else if (in_degree == 1 && out_degree > 1) {
        in1outN.insert(vertex);
    } else if (in_degree == 1 && out_degree == 1) {
        in1out1.insert(vertex);
    }
}

// 合并 in1out1 节点
void Compression::mergeIn1Out1Nodes() {
    std::vector<int> nodesToRemove;

    for (int node : in1out1) {
        if (g.vertices[node].LIN.size() == 1) {
            int predecessor = g.vertices[node].LIN.front();
            mergeNodes(predecessor, node);

            in1outN.erase(predecessor);
            inNout1.erase(predecessor);
            in1out1.erase(predecessor);

            classifyVertex(predecessor);
            nodesToRemove.push_back(node);
        }
    }

    for (int node : nodesToRemove) {
        in1out1.erase(node);
    }
}
