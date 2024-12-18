#ifndef CAL_RATIO_H
#define CAL_RATIO_H

#include <string>
#include <vector>
#include <queue>
#include <unordered_set>
#include <fstream>
#include "graph.h"

using namespace std;

class Graph;

// 获取当前时间戳
std::string getCurrentTimestamp();

// 拓扑排序 (Kahn 算法)
bool topologicalSort(const Graph& g, std::vector<int>& topoOrder);

// 计算可达性比例
double computeReachRatio(const Graph& g, const vector<int>& topoOrder);

#endif // CAL_RATIO_H