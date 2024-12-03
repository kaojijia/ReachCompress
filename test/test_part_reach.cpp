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


// 使用命名空间简化代码
using namespace std;


class ReachabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化代码
        InputHandler inputHandler(PROJECT_ROOT_DIR "/Edges/ia-radoslaw-email_edges.txt");
        inputHandler.readGraph(g);
        OutputHandler outputHandler(PROJECT_ROOT_DIR "/result/20241203");
        
    }

    void TearDown() override {
        // 清理代码
    }

    Graph g;
};


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
TEST_F(ReachabilityTest, BasicTest) {
    CompressedSearch comps(g);
    comps.offline_industry();
    string filename = "ia-radoslaw-email_edges.txt";
    OutputHandler outputHandler(PROJECT_ROOT_DIR "/result/20241203");
    outputHandler.writeStringWithTimestamp("当前处理文件"+filename);
    outputHandler.writeStringWithTimestamp("完成Compress离线索引构建");
    
    PLL pll(g);
    pll.offline_industry();
    outputHandler.writeStringWithTimestamp("完成pll离线索引构建");
    
    auto pllIndexSizes = pll.getIndexSizes();
    std::ostringstream oss;
    oss << "PLL Index Sizes - IN: " << pllIndexSizes["IN"] << ", OUT: " << pllIndexSizes["OUT"];
    outputHandler.writeStringWithTimestamp(oss.str());

    int num_queries = 100;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);

    // 计算每个方法的平均耗时
    long long total_duration_bfs = 0;
    long long total_duration_part = 0;
    long long total_duration_pll = 0;
    int query_count = 0;

    for (const auto& query_pair : query_pairs) {
        int source = query_pair.first;
        int target = query_pair.second;

        std::ostringstream query_oss;
        query_oss << "Query from " << source << " to " << target;
        outputHandler.writeStringWithTimestamp(query_oss.str());

        // 测量 BiBFS 查询耗时
        auto start_bfs = std::chrono::high_resolution_clock::now();
        bool bfs_result = comps.bfs.reachability_query(source, target);
        auto end_bfs = std::chrono::high_resolution_clock::now();
        auto duration_bfs = std::chrono::duration_cast<std::chrono::microseconds>(end_bfs - start_bfs).count();
        total_duration_bfs += duration_bfs;

        // 测量 Partitioned Search 查询耗时
        auto start_part = std::chrono::high_resolution_clock::now();
        bool part_result = comps.reachability_query(source, target);
        auto end_part = std::chrono::high_resolution_clock::now();
        auto duration_part = std::chrono::duration_cast<std::chrono::microseconds>(end_part - start_part).count();
        total_duration_part += duration_part;

        // 测量 PLL 查询耗时
        auto start_pll = std::chrono::high_resolution_clock::now();
        bool pll_result = pll.reachability_query(source, target);
        auto end_pll = std::chrono::high_resolution_clock::now();
        auto duration_pll = std::chrono::duration_cast<std::chrono::microseconds>(end_pll - start_pll).count();
        total_duration_pll += duration_pll;

        // 输出查询结果和耗时
        std::ostringstream result_oss;
        result_oss << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)";
        outputHandler.writeStringWithTimestamp(result_oss.str());

        result_oss.str(""); // 清空字符串流
        result_oss << "PartSearch: " << (part_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_part << " microseconds)";
        outputHandler.writeStringWithTimestamp(result_oss.str());

        result_oss.str(""); // 清空字符串流
        result_oss << "PLLSearch: " << (pll_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_pll << " microseconds)";
        outputHandler.writeStringWithTimestamp(result_oss.str());

        query_count++;

        bool results_match = (bfs_result == part_result) && (bfs_result == pll_result);
        std::ostringstream match_oss;
        match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
        outputHandler.writeStringWithTimestamp(match_oss.str());

        EXPECT_EQ(bfs_result, part_result);
        EXPECT_EQ(bfs_result, pll_result);
    }

    std::ostringstream final_oss;
    final_oss << "Final Average BiBFS Time: " << (total_duration_bfs / query_count) << " microseconds";
    outputHandler.writeStringWithTimestamp(final_oss.str());

    final_oss.str(""); // 清空字符串流
    final_oss << "Final Average PartSearch Time: " << (total_duration_part / query_count) << " microseconds";
    outputHandler.writeStringWithTimestamp(final_oss.str());

    final_oss.str(""); // 清空字符串流
    final_oss << "Final Average PLLSearch Time: " << (total_duration_pll / query_count) << " microseconds";
    outputHandler.writeStringWithTimestamp(final_oss.str());
}


TEST(ReachabilityTest, DISABLED_ReachabilityRatioTest){
    Graph g(true);  // 确保存储边集    
    //InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241031_163820");
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/soc-Epinions1.txt");
    inputHandler.readGraph(g);
    //OutputHandler::printGraphInfo(g);
    CompressedSearch comps(g);
    comps.offline_industry();
    float ratio = compute_reach_ratio(g);
    std::cout << "reach ratio: " << ratio << std::endl;
    auto pm = comps.get_partition_manager();
    // OutputHandler::printPartitionInfo(pm);
    for (const auto& partition : pm.get_mapping()) {
        int partition_id = partition.first;
        float partition_ratio = compute_reach_ratio(pm, partition_id);
        std::cout << "Partition " << partition_id << " reach ratio: " << partition_ratio << std::endl;
    }
}


TEST(ReachabilityTest, DISABLED_TotalReachabilityRatioTest) {
    string edgesDirectory = string(PROJECT_ROOT_DIR) + "/Edges";  // 根据实际路径修改
    string outputFilePath = "reach_ratio_results.csv";            // 输出文件路径

    // 获取所有边文件
    vector<string> edgeFiles = getAllFiles(edgesDirectory);

    if (edgeFiles.empty()) {
        cout << "没有找到任何边文件。" << endl;
        return;
    }

    for (const string& edgeFile : edgeFiles) {
        // 输出找到的边文件
        cout << "找到边文件：" << edgeFile << endl;
    }

    // 打开输出文件
    ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        cout << "无法打开输出文件: " << outputFilePath << endl;
        return;
    }

    // 写入CSV头
    outputFile << "File,TotalReachRatio,PartitionID,PartitionReachRatio\n";

    // 遍历每个边文件
    for (const auto& edgeFilePath : edgeFiles) {
        cout << "正在处理: " << edgeFilePath << endl;

        // 初始化图
        Graph g(true);  // 确保存储边集

        // 读取边文件
        InputHandler inputHandler(edgeFilePath);
        inputHandler.readGraph(g);

        // 输出图信息（可选）
        // OutputHandler::printGraphInfo(g);

        // 初始化 CompressedSearch 并进行离线处理
        CompressedSearch comps(g);
        comps.offline_industry();
        cout<<"完成离线索引构建"<<endl;
        // 计算全图的可达性比例
        float total_ratio = compute_reach_ratio(g);
        cout << "Total reach ratio: " << total_ratio << endl;

        // 获取分区管理器并输出分区信息
        PartitionManager pm = comps.get_partition_manager();
        // OutputHandler::printPartitionInfo(pm); // 如果需要，可以取消注释

        // 遍历每个分区并计算可达性比例
        for (const auto& partition : pm.get_mapping()) {
            int partition_id = partition.first;
            float partition_ratio = compute_reach_ratio(pm, partition_id);
            cout << "Partition " << partition_id << " reach ratio: " << partition_ratio << endl;

            // 写入结果到CSV
            outputFile << "\"" << edgeFilePath << "\"," << total_ratio << "," << partition_id << "," << partition_ratio << "\n";
        }

        // 如果没有分区，仍然记录全图的可达性比例
        if (pm.get_mapping().empty()) {
            outputFile << "\"" << edgeFilePath << "\"," << total_ratio << ",N/A,N/A\n";
        }

    }

    // 关闭输出文件
    outputFile.close();

    cout << "Reachability ratio testing completed. Results saved to " << outputFilePath << endl;

    // 断言（根据需求添加）
    // 例如：EXPECT_GE(total_ratio, 0.0f);
}