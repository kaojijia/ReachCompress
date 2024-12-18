// ReachRatio.h
#ifndef REACH_RATIO_H
#define REACH_RATIO_H

#include "PartitionManager.h"
#include <unordered_set>

// 计算所有点对之间的可达性比（弗洛伊德算法）
// 参数：
// - pm：分区管理器对象
// - target_partition：目标分区ID
// 返回：可达性比例（float）
std::unordered_map<int, float> compute_reach_ratios(const PartitionManager& pm);


// 计算指定顶点集合之间的可达性比例（弗洛伊德算法）
// 参数：
// - graph：图对象
// 返回：可达性比例（float）
float compute_reach_ratio(Graph& graph);

//使用pll方法计算全图可达比例
float compute_reach_ratio_bfs(Graph& graph);


#endif // REACH_RATIO_H