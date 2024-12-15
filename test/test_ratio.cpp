#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "LouvainPartitioner.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include "pll.h"
#include "CompressedSearch.h"
#include "ReachRatio.h"
#include <dirent.h>      // 用于目录遍历
#include <sys/types.h>   // 定义了DIR等数据类型
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

// 使用命名空间简化代码
using namespace std;

// 获取当前时间戳的辅助函数
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time);
#else
    localtime_r(&now_time, &local_tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string getCurrentDaystamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time);
#else
    localtime_r(&now_time, &local_tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&local_tm, "%Y%m%d");
    return ss.str();
}


// 辅助函数：获取指定目录下的所有文件路径
vector<string> getAllFiles(const string& directoryPath) {
    vector<string> files;
    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(directoryPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            // 跳过当前目录和父目录
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            string fileName = ent->d_name;
            // 根据需要过滤文件类型，例如只处理 .txt 或 .edge 文件
            // 这里假设所有文件都是有效的边文件
            // 如果有特定的扩展名，可以添加判断
            // 比如：
            // if (fileName.substr(fileName.find_last_of(".") + 1) == "txt") {
            //     files.push_back(directoryPath + "/" + fileName);
            // }
            files.push_back(directoryPath + "/" + fileName);
        }
        closedir(dir);
    } else {
        // 目录无法打开
        perror("无法打开目录");
    }

    return files;
}

TEST(ReachabilityTest, TotalReachabilityRatioTest) {
    string edgesDirectory = PROJECT_ROOT_DIR"/Edges/DAGs/medium";  // 根据实际路径修改
    
    string outputFilePath = string(PROJECT_ROOT_DIR)+"/result/"+getCurrentDaystamp()+"/reach_ratio_results_2.csv";      

    string file2 = "/home/reco/Projects/ReachCompress/Edges/large/tweibo-edgelist.txt";
    // 获取所有边文件
    // vector<string> edgeFiles = getAllFiles(edgesDirectory);

    vector<string> edgeFiles; 
    edgeFiles.push_back(file2);
    if (edgeFiles.empty()) {
        cout << "[" << getCurrentTimestamp() << "] " << "没有找到任何边文件。" << endl;
        return;
    }

    for (const string& edgeFile : edgeFiles) {
        // 输出找到的边文件
        cout << "[" << getCurrentTimestamp() << "] " << "找到边文件：" << edgeFile << endl;
    }

    // 打开输出文件
    ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        cout << "[" << getCurrentTimestamp() << "] " << "无法打开输出文件: " << outputFilePath << endl;
        return;
    }

    // 写入CSV头
    outputFile << "File,TotalReachRatio\n";

    // 遍历每个边文件
    for (const auto& edgeFilePath : edgeFiles) {
        cout << "[" << getCurrentTimestamp() << "] " << "正在处理: " << edgeFilePath << endl;
        // 初始化图
        Graph g(true);  // 确保存储边集

        // 读取边文件
        InputHandler inputHandler(edgeFilePath);
        inputHandler.readGraph(g);

        BidirectionalBFS bfs(g);
        // bfs.set_reachable_matrix();
        // 输出图信息（可选）
        // OutputHandler::printGraphInfo(g);

        // 计算全图的可达性比例
        float total_ratio = compute_reach_ratio_pll(g);
        cout << "[" << getCurrentTimestamp() << "] " << "Total reach ratio: " << total_ratio << endl;

        // 写入结果到CSV
        outputFile << "\"" << edgeFilePath << "\"," << total_ratio << "\n";
    }

    // 关闭输出文件
    outputFile.close();

    cout << "[" << getCurrentTimestamp() << "] " << "Reachability ratio testing completed. Results saved to " << outputFilePath << endl;

    // 断言（根据需求添加）
    // 例如：EXPECT_GE(total_ratio, 0.0f);
}