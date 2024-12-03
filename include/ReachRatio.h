// ReachRatio.h
#ifndef REACH_RATIO_H
#define REACH_RATIO_H

#include "PartitionManager.h"
#include <unordered_set>

// 计算所有点对之间的可达性比例（弗洛伊德算法）
// 参数：
// - pm：分区管理器对象
// - target_partition：目标分区ID
// 返回：可达性比例（float）
float compute_reach_ratio(const PartitionManager& pm, int target_partition);


// 计算指定顶点集合之间的可达性比例（弗洛伊德算法）
// 参数：
// - graph：图对象
// 返回：可达性比例（float）
float compute_reach_ratio(const Graph& graph);



#endif // REACH_RATIO_H