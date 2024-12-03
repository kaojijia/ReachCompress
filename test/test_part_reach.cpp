#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "LouvainPartitioner.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include "CompressedSearch.h"
#include "ReachRatio.h"
#include <dirent.h>      // 用于目录遍历
#include <sys/types.h>   // 定义了DIR等数据类型
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// 使用命名空间简化代码
using namespace std;

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
TEST(ReachabilityTest, DISABLED_BasicTest) {
    // 创建一个图
    Graph g(true);  // 确保存储边集    
    InputHandler inputHandler(PROJECT_ROOT_DIR"/Edges/generate/gene_edges_20241029_135003");
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);
    // // 创建PartitionManager对象
    // PartitionManager partition_manager(g);
    // // 进行分区
    // LouvainPartitioner partitioner;
    // partitioner.partition(g, partition_manager);
    
    // auto edges = partition_manager.get_partition_adjacency(3,16);
   
    // OutputHandler::printPartitionInfo(partition_manager);

    CompressedSearch comps(g);
    comps.offline_industry();
    // EXPECT_TRUE(comps.reachability_query(3,16));
    // EXPECT_TRUE(comps.reachability_query(3,18));
    // EXPECT_TRUE(comps.reachability_query(3,11));
    
    // EXPECT_FALSE(comps.reachability_query(3,0));

    // 这几个出问题了
    // comps.reachability_query(3,1);
    // comps.reachability_query(3,8);
    // comps.reachability_query(3,9);
    // comps.reachability_query(3,19);
    // comps.reachability_query(3,12);
    // comps.reachability_query(3,7);
    // comps.reachability_query(0,1);
    // comps.reachability_query(0,17);
    // comps.reachability_query(0,17);

    int num_queries = 100;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);


    for (const auto& query_pair : query_pairs) {
        int source = query_pair.first;
        int target = query_pair.second;

        std::cout << "query from "<< source << " to " <<target<<std::endl;
        bool bfs_result = comps.bfs.reachability_query(source, target);
        bool part_result = comps.reachability_query(source, target);
        EXPECT_EQ(bfs_result,part_result);
        std::cout << "BiBFS :"<< (bfs_result ? " Reachable" : " Not Reachable") << std::endl;
        std::cout << "PartSearch:" << (part_result ? " Reachable" : " Not Reachable") << std::endl;
        std::cout << "***** Does Result Match from " << source << " query to " << target << ":  " << ((bfs_result==part_result)?"Match":"Not Match") <<" *****"<< std::endl;
        std::cout << std::endl;
        // if (!path.empty()) {
        //     std::cout << "Path found: ";
        //     for (int node : path) {
        //         std::cout << node << " ";
        //     }
        //     std::cout << std::endl;
        // } else {
        //     std::cout << "No path exists" << std::endl;
        // }

    }



}


TEST(ReachabilityTest, ReachabilityRatioTest){
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