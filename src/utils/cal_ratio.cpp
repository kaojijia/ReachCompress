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
#include <set>


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
// double computeReachRatio(const Graph& g, const vector<int>& topoOrder) {
//     int n = g.vertices.size();
//     vector<set<int>> reachableSets(n); // 每个节点的可达节点集合

//     // 反向拓扑排序遍历
//     for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
//         int node = *it;

//         // 遍历所有后继节点，合并可达节点集合
//         for (int neighbor : g.vertices[node].LOUT) {
//             reachableSets[node].insert(neighbor); // 直接加入后继节点本身
//             reachableSets[node].insert(reachableSets[neighbor].begin(), reachableSets[neighbor].end());
//         }
//     }

//     // 计算总的可达点对数
//     long long reachablePairs = 0;
//     for (const auto& reachableSet : reachableSets) {
//         reachablePairs += reachableSet.size();
//     }

//     n = g.get_num_vertices();
//     // 计算可达性比例
//     long long totalPairs = static_cast<long long>(n) * (n - 1); // 排除自身
//     return totalPairs > 0 ? static_cast<double>(reachablePairs) / static_cast<double>(totalPairs) : 0.0;
// }

#include <unordered_set>

double computeReachRatio(const Graph& g, const vector<int>& topoOrder) {
    
    string logFilePath = PROJECT_ROOT_DIR "/ratio_log.txt";
    ofstream logFile(logFilePath, ios::out);
    int n = g.vertices.size();
    vector<int*> reachableSets(n, nullptr); // 每个节点的可达节点数组
    vector<int> reachableSizes(n, 0);      // 每个节点的可达节点数组大小
    int num = g.get_num_vertices();
    // 反向拓扑排序遍历
    int count = 0;
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        int node = *it;
        if(++count%10000==1){
            cout<<getCurrentTimestamp()<<"  计算到第"<<count<<"个节点reachableSets"<<endl;
            logFile<<getCurrentTimestamp()<<"  计算到第"<<count<<"个节点reachableSets"<<endl;
        }
        
        // 使用unordered_set去重计算可达节点集合
        std::unordered_set<int> uniqueReachable;
        
        // 包含后继节点自身
        for (int neighbor : g.vertices[node].LOUT) {
            if (neighbor >= 0 && neighbor < n) { // 合法性检查
                uniqueReachable.insert(neighbor);
                for (int j = 0; j < reachableSizes[neighbor]; ++j) {
                    uniqueReachable.insert(reachableSets[neighbor][j]);
                }
            }
        }

        // 计算去重后的集合大小
        int totalSize = uniqueReachable.size();

        // 回收当前节点旧的数组（如果存在）
        if (reachableSets[node] != nullptr) {
            delete[] reachableSets[node];
        }

        // 分配新的数组，存储所有可达节点
        reachableSets[node] = new int[totalSize];
        int* curArray = reachableSets[node];
        reachableSizes[node] = totalSize;

        // 将去重后的节点存入数组
        int idx = 0;
        for (int reachableNode : uniqueReachable) {
            if (idx < totalSize) { // 防止越界写入
                curArray[idx++] = reachableNode;
            }
        }
    }

    logFile.close();

    // 计算总的可达点对数
    long long reachablePairs = 0;
    for (int i = 0; i < n; ++i) {
        reachablePairs += reachableSizes[i];
    }

    // 清理内存
    for (int i = 0; i < n; ++i) {
        if (reachableSets[i] != nullptr) {
            delete[] reachableSets[i];
            reachableSets[i] = nullptr;
        }
    }

    // 计算可达性比例
    long long totalPairs = static_cast<long long>(num) * (num - 1); // 排除自身
    return totalPairs > 0 ? static_cast<double>(reachablePairs) / static_cast<double>(totalPairs) : 0.0;
}


