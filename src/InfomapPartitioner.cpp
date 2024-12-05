#include "InfomapPartitioner.h"
#include <fstream>
#include <sstream>
#include <iostream>

void InfomapPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    // 获取图的文件名
    std::string graph_filename = graph.getFilename();
    if (graph_filename.empty()) {
        std::cerr << "Graph filename is empty." << std::endl;
        return;
    }

    // 分割出文件名
    size_t last_slash_idx = graph_filename.find_last_of("\\/");
    if (std::string::npos != last_slash_idx) {
        graph_filename.erase(0, last_slash_idx + 1);
    }
    size_t period_idx = graph_filename.rfind('.');
    if (std::string::npos != period_idx) {
        graph_filename.erase(period_idx);
    }

    // 构建分区文件名
    std::string partition_filename = PROJECT_ROOT_DIR"/Partitions/"+graph_filename + "_partitions.txt";

    // 打开分区文件
    std::ifstream partition_file(partition_filename);
    if (!partition_file.is_open()) {
        std::cerr << "Failed to open partition file: " << partition_filename << std::endl;
        return;
    }

    // 读取分区文件并更新 Graph
    std::string line;
    while (std::getline(partition_file, line)) {
        std::istringstream iss(line);
        int node, partition_id;
        if (!(iss >> node >> partition_id)) {
            std::cerr << "Invalid line format in partition file: " << line << std::endl;
            continue;
        }

        // 更新 Graph 中的 vertices
        if (!graph.set_partition_id(node, partition_id)) {
            std::cerr << "Failed to set partition id for node: " << node << std::endl;
            continue;
        }

    }

    partition_file.close();
    std::cout << "Partitioning completed using file: " << partition_filename << std::endl;
    
    // 建立分区图和对应的信息
    partition_manager.build_partition_graph();
}