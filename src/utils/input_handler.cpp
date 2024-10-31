#include "utils/InputHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>

InputHandler::InputHandler(const std::string& input_file) : input_file(input_file) {}

// 从文件中读取边集并构建图
void InputHandler::readGraph(Graph& g) {
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file: " << input_file << std::endl;
        return;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int u, v;
        if (!(iss >> u >> v)) {
            continue;  // 跳过无效行
        }
        g.addEdge(u, v);
    }
    infile.close();
}
