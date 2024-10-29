#include "OutputHandler.h"
#include <fstream>
#include <iostream>

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

// 非静态方法：写入图的信息到文件
OutputHandler::OutputHandler(const std::string& output_file) : output_file(output_file) {}

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