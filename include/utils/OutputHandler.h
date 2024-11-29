#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#include <string>
#include <vector>
#include "graph.h"
#include "pll.h"
#include "compression.h"
#include "PartitionManager.h"

class OutputHandler {
public:
    // 静态方法：输出图的信息到控制台
    static void printGraphInfo(const Graph& graph);
    static void printMapping(const Compression& com);
    static void printPartitionInfo(const PartitionManager& partition_manager);


    // 非静态方法：写入图的信息到文件
    explicit OutputHandler(const std::string& output_file);

    void writeGraphInfo(const Graph& graph);
    void writeCoverInfo(const std::vector<int>& cover);
    void writereachability_query(int u, int v, bool result);
    void writeInOutSets(const PLL& pll);
    // void writePartitions(const PartitionManager& partition_manager);


private:
    std::string output_file;  // 输出文件路径
};

#endif  // OUTPUT_HANDLER_H