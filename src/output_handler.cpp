#include "OutputHandler.h"
#include <fstream>
#include <iostream>

OutputHandler::OutputHandler(const std::string& output_file) : output_file(output_file) {}

// 写入2-hop覆盖信息
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

// 写入可达性查询结果
void OutputHandler::writeReachabilityQuery(int u, int v, bool result) {
    std::ofstream outfile(output_file, std::ios_base::app);  // 以追加方式打开文件
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    outfile << "Reachability Query: From " << u << " to " << v << " - ";
    outfile << (result ? "Reachable" : "Not Reachable") << std::endl;
    outfile.close();
}
