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
    const auto& partition_adjacency = partition_manager.partition_adjacency;

    for (const auto& source_pair : partition_adjacency) {
        int source_partition = source_pair.first;
        const auto& targets = source_pair.second;
        for (const auto& target_pair : targets) {
            int target_partition = target_pair.first;
            const auto& edge_info = target_pair.second;
            std::cout << "Partition " << source_partition << " -> Partition " << target_partition << ": " << edge_info.edge_count << " edges" << std::endl;
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
