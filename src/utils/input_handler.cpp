#include "utils/InputHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
using namespace std;
InputHandler::InputHandler(const std::string& input_file) : input_file(input_file) {}

// 从文件中读取边集并构建图
void InputHandler::readGraph(Graph& g) {
    std::ifstream infile(input_file);
    if (!infile.is_open()) {
        std::cerr << "Error opening input file: " << input_file << std::endl;
        return;
    }
    int num = 0;
    std::string line;
    cout<<"开始读取文件"<<endl;
    uint32_t max_node_id = 0;
    while(getline(infile, line)){
        std::istringstream iss(line);
        uint32_t u, v;
        if (!(iss >> u >> v)) {
            continue;  // 跳过无效行
        }
        max_node_id = max(max_node_id, max(u, v));
    }
    g.set_max_node_id(max_node_id);
    infile.clear();
    infile.seekg(0);
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int u, v;
        if (!(iss >> u >> v)) {
            continue;  // 跳过无效行
        }
        g.addEdge(u, v);
        if(++num % 10000 == 0)cout<<"已经添加"<<num<<"条边"<<endl;
    }
    infile.close();
    cout<<"读取文件结束"<<endl;
    cout<<"节点数量为"<<g.get_num_vertices()<<endl;
    cout<<"边数量为"<<g.get_num_edges()<<endl;
}
