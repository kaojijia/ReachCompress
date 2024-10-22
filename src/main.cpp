#include "pll.h"
#include "graph_pruner.h"
#include "compression.h"
#include "InputHandler.h"
#include "OutputHandler.h"
#include "BidirectionalBFS.h"
#include <iostream>

int main() {
    // 输入文件路径和输出文件路径可以在运行时或配置中指定
    InputHandler input_handler("input.txt");
    OutputHandler output_handler("output.txt");


    // 初始化图
    Graph g;
    
    // 从文件中读取边集
    input_handler.readGraph(g);

    // 使用双向BFS进行可达性查询
    BidirectionalBFS bfs(g);

    // 实例化GraphPruner并裁剪图
    GraphPruner pruner(g);
    pruner.pruneIsolatedNodes();  // 裁剪孤立节点

    // 实例化PLL并构建索引
    PLL pll(g);
    pll.buildPLLLabels();

    // 查询可达性并写入结果
    int u = 1, v = 5;
    bool reachability_result = pll.reachabilityQuery(u, v);
    output_handler.writeReachabilityQuery(u, v, reachability_result);

    // 获取2-Hop覆盖并写入结果
    std::vector<int> cover = pll.greedy2HopCover();
    output_handler.writeCoverInfo(cover);

    return 0;
}
