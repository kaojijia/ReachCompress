#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include "graph.h"
#include "pll.h"
#include "compression.h"
#include "PartitionManager.h"
#include <chrono>
#include <iomanip>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

class OutputHandler {
public:
    // 静态方法：输出图的信息到控制台
    static void printGraphInfo(const Graph& graph);
    static void printMapping(const Compression& com);
    static void printPartitionInfo(const PartitionManager& partition_manager);


    // 非静态方法：写入图的信息到文件
    explicit OutputHandler(const std::string& output_file);

    void writeStringWithTimestamp(const std::string &str);

    void writeString(const std::string& str);
    void writeGraphInfo(const Graph& graph);
    void writeCoverInfo(const std::vector<int>& cover);
    void writereachability_query(int u, int v, bool result);
    void writeInOutSets(const PLL& pll);
    // void writePartitions(const PartitionManager& partition_manager);

    ~OutputHandler();
private:
    std::string output_file;  // 输出文件路径
    void flushBuffer(); // 刷新缓冲区
    void backgroundFlush(); // 后台线程刷新

    std::ofstream outfile_; // 持久化的文件流
    std::vector<std::string> buffer_; // 写入缓冲区
    std::mutex mtx_;
    std::condition_variable cv_;
    bool exit_flag_;
    std::thread flush_thread_;
};

#endif  // OUTPUT_HANDLER_H