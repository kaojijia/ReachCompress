#include "utils/OutputHandler.h"
#include <fstream>
#include <iostream>
#include "compression.h"
#include "PartitionManager.h"


// 静态方法：输出图的信息到控制台
void OutputHandler::printGraphInfo(const Graph& graph) {
    std::cout << "source target" << std::endl;
    for (const auto& vertex : graph.vertices) {
        int source = &vertex - &graph.vertices[0];  // 获取节点的索引
        for (int target : vertex.LOUT) {
            std::cout << source << " " << target << std::endl;
        }
    }
}

// 打印 Compression 对象中的 mapping
void OutputHandler::printMapping(const Compression &com) {
    const auto& mapping = com.getMapping();

    for (const auto& pair : mapping) {
        std::cout << "Node " << pair.first << ":" << std::endl;

        std::cout << "  SCC Mapping: ";
        for (int mapped_node : pair.second.sccMapping) {
            std::cout << mapped_node << " ";
        }
        std::cout << std::endl;

        std::cout << "  Out Mapping: ";
        for (int mapped_node : pair.second.outMapping) {
            std::cout << mapped_node << " ";
        }
        std::cout << std::endl;

        std::cout << "  In Mapping: ";
        for (int mapped_node : pair.second.inMapping) {
            std::cout << mapped_node << " ";
        }
        std::cout << std::endl;
    }
}

void OutputHandler::printPartitionInfo(const PartitionManager &partition_manager)
{
    const auto& mapping = partition_manager.get_mapping();

    for (const auto& pair : mapping) {
        if(pair.first == -1) continue;
        std::cout << "Partition " << pair.first << ":" << std::endl;
        std::cout << "  Nodes: ";
        for (int node : pair.second) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
}

// 非静态方法：写入图的信息到文件

OutputHandler::OutputHandler(const std::string& output_file)
    : output_file(output_file), outfile_(output_file, std::ios_base::app), exit_flag_(false)
{
    if (!outfile_.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
    }
    // 启动后台刷新线程
    flush_thread_ = std::thread(&OutputHandler::backgroundFlush, this);
}


void OutputHandler::writeStringWithTimestamp(const std::string &str)
{
    if (!outfile_.is_open()) {
        std::cerr << "Output file is not open: " << output_file << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_us.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    long duration = value.count();

    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(6) << (duration % 1000000) << " ";
    oss << str << "\n";

    {
        std::unique_lock<std::mutex> lock(mtx_);
        buffer_.emplace_back(oss.str());
    }
    cv_.notify_one();
}
void OutputHandler::flushBuffer()
{
    std::vector<std::string> temp;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        temp.swap(buffer_);
    }

    for (const auto& line : temp) {
        outfile_ << line;
    }
    outfile_.flush();
}

void OutputHandler::backgroundFlush()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (cv_.wait_for(lock, std::chrono::milliseconds(100), [this]() { return !buffer_.empty() || exit_flag_; })) {
            if (exit_flag_ && buffer_.empty()) {
                break;
            }
            flushBuffer();
        }
    }
}
void OutputHandler::writeString(const std::string &str)
{
    std::ofstream outfile(output_file, std::ios_base::out);  // 以覆盖方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    outfile << str << std::endl;
    outfile.close();
}

void OutputHandler::writeGraphInfo(const Graph& graph) {


    std::ofstream outfile(output_file, std::ios_base::app);  // 以追加方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    // 写入图的边信息
    outfile << "source target" << std::endl;
    for (const auto& vertex : graph.vertices) {
        int source = &vertex - &graph.vertices[0];  // 获取节点的索引
        for (int target : vertex.LOUT) {
            outfile << source << " " << target << std::endl;
        }
    }
    outfile.close();
}

void OutputHandler::writeCoverInfo(const std::vector<int>& cover) {
    std::ofstream outfile(output_file, std::ios_base::app);  // 以追加方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    outfile << "2-hop Cover Nodes: ";
    for (int node : cover) {
        outfile << node << " ";
    }
    outfile << std::endl;
    outfile.close();
}

void OutputHandler::writereachability_query(int u, int v, bool result) {
    std::ofstream outfile(output_file, std::ios_base::app);  // 以追加方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    outfile << "Reachability Query: From " << u << " to " << v << " - ";
    outfile << (result ? "Reachable" : "Not Reachable") << std::endl;
    outfile.close();
}


void OutputHandler::writeInOutSets(const PLL &pll) {
    std::ofstream outfile(output_file, std::ios_base::out);  // 以覆盖方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    for (int i = 0; i < pll.IN.size(); ++i) {
        outfile << "Node " << i << " IN_set: ";
        for (int inNode : pll.IN[i]) {
            outfile << inNode << " ";
        }
        outfile << std::endl;

        outfile << "Node " << i << " OUT_set: ";
        for (int outNode : pll.OUT[i]) {
            outfile << outNode << " ";
        }
        outfile << std::endl;
    }
    outfile.close();
}



OutputHandler::~OutputHandler() {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        exit_flag_ = true;
        cv_.notify_all();
    }
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    // 最后一次刷新
    flushBuffer();
    if (outfile_.is_open()) {
        outfile_.close();
    }
}