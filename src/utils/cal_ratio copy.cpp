#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "graph.h"  
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"


using namespace std;

// 获取当前时间戳
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
    return ss.str();
}

// 拓扑排序 (Kahn 算法)
bool topologicalSort(const Graph& g, vector<int>& topoOrder) {
    int n = g.vertices.size();
    vector<int> inDegree(n, 0);

    // 计算每个节点的入度
    for (const auto& v : g.vertices) {
        for (int neighbor : v.LOUT) {
            inDegree[neighbor]++;
        }
    }

    queue<int> zeroInDegree;
    for (int i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            zeroInDegree.push(i);
        }
    }

    while (!zeroInDegree.empty()) {
        int node = zeroInDegree.front();
        zeroInDegree.pop();
        topoOrder.push_back(node);

        for (int neighbor : g.vertices[node].LOUT) {
            if (--inDegree[neighbor] == 0) {
                zeroInDegree.push(neighbor);
            }
        }
    }

    return topoOrder.size() == n;
}

// 计算可达性比例（存储可达节点数量）
// 计算可达性比例（使用 unordered_set 避免重复）
double computeReachRatio(const Graph& g, const vector<int>& topoOrder) {
    int n = g.vertices.size();
    vector<unordered_set<int>> reachableSets(n); // 每个节点的可达节点集合

    // 反向拓扑排序遍历
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        int node = *it;

        // 遍历所有后继节点，合并可达节点集合
        for (int neighbor : g.vertices[node].LOUT) {
            reachableSets[node].insert(neighbor); // 直接加入后继节点本身
            reachableSets[node].insert(reachableSets[neighbor].begin(), reachableSets[neighbor].end());
        }
    }

    // 计算总的可达点对数
    long long reachablePairs = 0;
    for (const auto& reachableSet : reachableSets) {
        reachablePairs += reachableSet.size();
    }

    n = g.get_num_vertices();
    // 计算可达性比例
    long long totalPairs = static_cast<long long>(n) * (n - 1); // 排除自身
    return totalPairs > 0 ? static_cast<double>(reachablePairs) / static_cast<double>(totalPairs) : 0.0;
}


int main() {
    // 输入与输出路径
    string inputFile = PROJECT_ROOT_DIR "/Edges/DAGs/large/tweibo-edgelist_DAG";
    string outputFile = PROJECT_ROOT_DIR "/ratio_results.txt";

    // 读取图
    cout << getCurrentTimestamp() << "  开始读取图数据: " << inputFile << endl;
    Graph g(false);
    InputHandler inputHandler(inputFile);
    inputHandler.readGraph(g);
    cout << getCurrentTimestamp() << "  图数据读取完成。" << endl;

    // 执行拓扑排序
    vector<int> topoOrder;
    if (!topologicalSort(g, topoOrder)) {
        cerr << "图包含环，无法计算可达性比例: " << inputFile << endl;
        return -1;
    }

    // 计算可达性比例
    double reachRatio = computeReachRatio(g, topoOrder);
    cout << getCurrentTimestamp() << "  文件: " << inputFile << ", 可达性比例: " << reachRatio << endl;

    // 输出结果
    ofstream outFile(outputFile, ios::app);
    if (outFile.is_open()) {
        outFile << getCurrentTimestamp() << "  文件: " << inputFile << ", 可达性比例: " << reachRatio << "\n";
        outFile.close();
    } else {
        cerr << "无法打开输出文件: " << outputFile << endl;
        return -1;
    }

    return 0;
}
