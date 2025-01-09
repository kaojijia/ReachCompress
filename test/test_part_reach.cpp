#include "gtest/gtest.h"
#include "graph.h"
#include "PartitionManager.h"
#include "partitioner/LouvainPartitioner.h"
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
#include <thread>


// 使用命名空间简化代码
using namespace std;


class ReachabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化代码
        // 获取所有边文件
        // string edgesDirectory = string(PROJECT_ROOT_DIR) + "/Edges/test";

        // // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/ia-radoslaw-email_edges.txt");
        // // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/Slashdot0811.txt");
        // // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/soc-LiveJournal1.txt");

        // edgeFiles = this->getAllFiles(edgesDirectory);
        // if (edgeFiles[0].empty()) {
        //     FAIL() << "没有找到任何边文件。";
        // }
    }

    void TearDown() override {
        // 清理代码
    }


    string getCurrentTimestamp() {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        tm local_tm;
    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_tm, &now_time);
    #else
        localtime_r(&now_time, &local_tm);
    #endif
        stringstream ss;
        ss << put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    string getCurrentDaystamp() {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        tm local_tm;
    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_tm, &now_time);
    #else
        localtime_r(&now_time, &local_tm);
    #endif
        stringstream ss;
        ss << put_time(&local_tm, "%Y%m%d");
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

    vector<pair<int, int>> readQueryPairs(const string& filePath, int distance, int num_pairs) {
        vector<pair<int, int>> query_pairs;
        ifstream file(filePath);
        if (!file.is_open()) {
            cerr << "无法打开文件: " << filePath << endl;
            return query_pairs;
        }

        string line;
        bool in_distance_section = false;
        while (getline(file, line)) {
            if (line == "Distance " + to_string(distance) + ":") {
                in_distance_section = true;
                continue;
            }
            if (in_distance_section) {
                if (line.empty()) {
                    break;
                }
                stringstream ss(line);
                int u, v;
                ss >> u >> v;
                query_pairs.emplace_back(u, v);
            }
        }

        file.close();

        // 随机选择 num_pairs 个 pair
        if (query_pairs.size() > num_pairs) {
            random_device rd;
            mt19937 g(rd());
            shuffle(query_pairs.begin(), query_pairs.end(), g);
            query_pairs.resize(num_pairs);
        }

        return query_pairs;
    }
    vector<pair<int, int>> readQueryPairs(const string& filePath, int distance) {
        vector<pair<int, int>> query_pairs;
        ifstream file(filePath);
        if (!file.is_open()) {
            cerr << "无法打开文件: " << filePath << endl;
            return query_pairs;
        }

        string line;
        bool in_distance_section = false;
        while (getline(file, line)) {
            if (line == "Distance " + to_string(distance) + ":") {
                in_distance_section = true;
                continue;
            }
            if (in_distance_section) {
                if (line.empty()) {
                    break;
                }
                stringstream ss(line);
                int u, v;
                ss >> u >> v;
                query_pairs.emplace_back(u, v);
            }
        }

        file.close();
        return query_pairs;
    }



    vector<string> edgeFiles;
};


// TEST_F(ReachabilityTest, BasicTest) {
//     // 打开日志文件
//     string logFilePath = string(PROJECT_ROOT_DIR) + "/result/"+getCurrentDaystamp()+"/Basictest_log.txt";
//     ofstream logFile(logFilePath, ios::out);
//     if (!logFile.is_open()) {
//         FAIL() << "无法打开日志文件: " << logFilePath;
//     }

//     // string edgeFile = PROJECT_ROOT_DIR"/Edges/medium/cit-DBLP";
//     // string edgefileDAG = PROJECT_ROOT_DIR"/Edges/DAGs/medium/cit-DBLP_DAG";
//     // string mapping = PROJECT_ROOT_DIR"/Edges/DAGmapping/cit-DBLP_mapping";


//     string edgeFile = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist";
//     string edgefileDAG = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist_DAG";
//     string mapping = PROJECT_ROOT_DIR"/Edges/DAGmapping/WikiTalk_edgelist_mapping_mapping";

//     cout << "Processing edge file: " << edgeFile << endl;
//     // 写入初始日志
//     logFile << "**************************************" << endl;
//     logFile << "*************************************" << endl;
//     logFile << "************************************" << endl;
//     logFile << "********************************" << endl;
//     logFile << "*************************" << endl;
//     logFile << "**************" << endl;
//     logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFile << endl;

//     // 初始化图
//     Graph g(true);  // 确保存储边集
//     Graph dag(true);
//     // 读取边文件
//     InputHandler inputHandler(edgeFile);
//     inputHandler.readGraph(g);
//     g.setFilename(edgeFile);
//     InputHandler inputHandlerDAG(edgefileDAG);
//     inputHandlerDAG.readGraph(dag);
//     dag.setFilename(edgefileDAG);

//     // 初始化 BFS
//     BidirectionalBFS bfs(g);

//     // 初始化 PLL 并进行离线处理    
//     PLL pll(g);
//     pll.offline_industry();
//     logFile << "[" << getCurrentTimestamp() << "] " << "完成 PLL 离线索引构建" << endl;

//     // 获取 PLL 索引大小并记录
//     auto pllIndexSizes = pll.getIndexSizes();
//     stringstream oss;
//     oss << "PLL Index Sizes - IN: " << pllIndexSizes["IN"] << ", OUT: " << pllIndexSizes["OUT"];
//     logFile << "[" << getCurrentTimestamp() << "] " << oss.str() << endl;


//     // 初始化 CompressedSearch 并进行离线处理
//     CompressedSearch comps(dag,"ReachRatio");
//     comps.offline_industry(200,0.3,mapping);
//     logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;

//     logFile << "Partition mapping:" << std::endl;
//     auto &pm = comps.get_partition_manager();
//     for (const auto& [partition, nodes] : pm.get_mapping()) {
//         if(partition == -1||nodes.size()<1) continue;
//         logFile << "Partition " << partition << ": ";
//         for (int node : nodes) {
//             logFile << node << " ";
//         }
//         logFile << std::endl;
//     }


//     logFile <<"     PLL index size: "<<endl;
//     auto indices =  pll.getIndexSizes();
//     for(auto &index: indices){
//         logFile << "[" << getCurrentTimestamp() << "] " << index.first << ": " << index.second << endl;
//     }
//     logFile <<"     Comps index size: "<<endl;
//     indices = comps.getIndexSizes();
//     for(auto &index: indices){
//         logFile << "[" << getCurrentTimestamp() << "] " << index.first << ": " << index.second << endl;
//     }


//     // // 生成查询对
//     // int num_queries = 1000;
//     // int max_value = g.vertices.size();
//     // unsigned int seed = 42; // 可选的随机种子

//     // //改成允许重复的 query
//     // vector<pair<int, int>> query_pairs = RandomUtils::generateQueryPairs(num_queries, max_value, seed);
//     string queryFile = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist_DAG_query.txt";

//     // 读取查询点对
//     vector<pair<int, int>> query_pairs_6 = readQueryPairs(queryFile, 6, 100);
//     vector<pair<int, int>> query_pairs_8 = readQueryPairs(queryFile, 8, 100);
//     vector<pair<int, int>> query_pairs_10 = readQueryPairs(queryFile, 10, 100);


//     // 计算每个方法的平均耗时
//     long long total_duration_bfs = 0;
//     long long total_duration_part = 0;
//     long long total_duration_pll = 0;
//     int query_count = 0;

//     for (const auto& query_pair : query_pairs) {
//         int source = query_pair.first;
//         int target = query_pair.second;

//         stringstream query_oss;

//         // 测量 BiBFS 查询耗时
//         auto start_bfs = chrono::high_resolution_clock::now();
//         bool bfs_result = bfs.reachability_query(source, target);
//         auto end_bfs = chrono::high_resolution_clock::now();
//         auto duration_bfs = chrono::duration_cast<chrono::microseconds>(end_bfs - start_bfs).count();


//         // 测量 Partitioned Search 查询耗时
//         auto start_part = chrono::high_resolution_clock::now();
//         bool part_result = comps.reachability_query(source, target);
//         auto end_part = chrono::high_resolution_clock::now();
//         auto duration_part = chrono::duration_cast<chrono::microseconds>(end_part - start_part).count();

//         // 测量 PLL 查询耗时
//         auto start_pll = chrono::high_resolution_clock::now();
//         bool pll_result = pll.reachability_query(source, target);
//         auto end_pll = chrono::high_resolution_clock::now();
//         auto duration_pll = chrono::duration_cast<chrono::microseconds>(end_pll - start_pll).count();

        

//         bool results_match = (bfs_result == part_result) && (bfs_result == pll_result);if(!results_match) continue;


//         query_oss << "Query from " << source << " to " << target;
//         logFile << "[" << getCurrentTimestamp() << "] " << query_oss.str() << endl;

//         query_count++;
//         total_duration_bfs += duration_bfs;
//         total_duration_part += duration_part;
//         total_duration_pll += duration_pll;
//         // 输出查询结果和耗时
//         stringstream result_oss;
//         result_oss << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)";
//         logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;

//         result_oss.str(""); // 清空字符串流
//         result_oss << "PartSearch: " << (part_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_part << " microseconds)";
//         logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;

//         result_oss.str(""); // 清空字符串流
//         result_oss << "PLLSearch: " << (pll_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_pll << " microseconds)";
//         logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;


//         stringstream match_oss;
//         match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
//         logFile << "[" << getCurrentTimestamp() << "] " << match_oss.str() << endl;

//         EXPECT_EQ(bfs_result, part_result);
//         EXPECT_EQ(bfs_result, pll_result);
//     }

//     // 计算并记录平均耗时
//     stringstream final_oss;
//     final_oss << "Final Average BiBFS Time: " << (query_count > 0 ? (total_duration_bfs / query_count) : 0) << " microseconds";
//     logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;

//     final_oss.str(""); // 清空字符串流
//     final_oss << "Final Average PartSearch Time: " << (query_count > 0 ? (total_duration_part / query_count) : 0) << " microseconds";
//     logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;

//     final_oss.str(""); // 清空字符串流
//     final_oss << "Final Average PLLSearch Time: " << (query_count > 0 ? (total_duration_pll / query_count) : 0) << " microseconds";
//     logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;

//     // 关闭日志文件
//     logFile.close();
// }

TEST_F(ReachabilityTest, DISABLED_IndexReachabilityTest) {

    size_t num_nodes = 300;
    float ratio = 0.9;

    // 打开日志文件
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/"+getCurrentDaystamp()+"/IndexReachability_log.txt";
    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }

    vector<string> edgeFile2 ;
    edgeFile2.push_back(string(PROJECT_ROOT_DIR) + "/Edges/medium/cit-DBLP");
   

    // 遍历每个边文件
    for (const auto& edgeFilePath : edgeFile2) {
        cout << "Processing edge file: " << edgeFilePath << endl;
        // 写入初始日志
        logFile << "**************************************" << endl;
        logFile << "*************************************" << endl;
        logFile << "************************************" << endl;
        logFile << "********************************" << endl;
        logFile << "*************************" << endl;
        logFile << "**************" << endl;
        logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFilePath << endl;

        // 初始化图
        Graph g(true);  // 确保存储边集

        // 读取边文件
        InputHandler inputHandler(edgeFilePath);
        inputHandler.readGraph(g);
        g.setFilename(edgeFilePath);
        // 初始化 CompressedSearch 并进行离线处理
        CompressedSearch comps(g,"Louvain");
        comps.offline_industry(num_nodes,ratio,nullptr);
        logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;
        auto lines = comps.get_index_info();
        for(auto line : lines){
            logFile<<line<<endl;
        }

        // 初始化 PLL 并进行离线处理
        PLL pll(g);
        pll.offline_industry();
        logFile << "[" << getCurrentTimestamp() << "] " << "完成 PLL 离线索引构建" << endl;

        // 获取 PLL 索引大小并记录
        auto pllIndexSizes = pll.getIndexSizes();
        stringstream oss;
        oss << "PLL Index Sizes - IN: " << pllIndexSizes[3].second << ", OUT: " << pllIndexSizes[4].second;
        logFile << "[" << getCurrentTimestamp() << "] " << oss.str() << endl;

        // 生成查询对
        int num_queries = 1000;
        int max_value = g.vertices.size();
        unsigned int seed = 42; // 可选的随机种子

        //改成允许重复的 query
        vector<pair<int, int>> query_pairs = RandomUtils::generateQueryPairs(num_queries, max_value, seed);
        vector<pair<int, int>> query_pairs2 = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);
        // 计算每个方法的平均耗时
        long long total_duration_bfs = 0;
        long long total_duration_part = 0;
        long long total_duration_pll = 0;
        int query_count = 0;



        for (const auto& query_pair : query_pairs2) {
            int source = query_pair.first;
            int target = query_pair.second;

            stringstream query_oss;

            // 测量 BiBFS 查询耗时
            auto start_bfs = chrono::high_resolution_clock::now();
            bool bfs_result = comps.bfs.reachability_query(source, target);
            auto end_bfs = chrono::high_resolution_clock::now();
            auto duration_bfs = chrono::duration_cast<chrono::microseconds>(end_bfs - start_bfs).count();


            // 测量 Partitioned Search 查询耗时
            auto start_part = chrono::high_resolution_clock::now();
            bool part_result = comps.reachability_query(source, target);
            auto end_part = chrono::high_resolution_clock::now();
            auto duration_part = chrono::duration_cast<chrono::microseconds>(end_part - start_part).count();

            // 测量 PLL 查询耗时
            auto start_pll = chrono::high_resolution_clock::now();
            bool pll_result = pll.reachability_query(source, target);
            auto end_pll = chrono::high_resolution_clock::now();
            auto duration_pll = chrono::duration_cast<chrono::microseconds>(end_pll - start_pll).count();

            cout << "BiBFS Time: " << duration_bfs << " microseconds" << endl;
            cout << "PartSearch Time: " << duration_part << " microseconds" << endl;
            cout << "PLLSearch Time: " << duration_pll << " microseconds" << endl;

            bool results_match = (bfs_result == part_result) && (bfs_result == pll_result);//if(!results_match) continue;


            query_oss << "Query from " << source << " to " << target;
            logFile << "[" << getCurrentTimestamp() << "] " << query_oss.str() << endl;

            query_count++;
            total_duration_bfs += duration_bfs;
            total_duration_part += duration_part;
            total_duration_pll += duration_pll;
            // 输出查询结果和耗时
            stringstream result_oss;
            result_oss << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)";
            logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;

            result_oss.str(""); // 清空字符串流
            result_oss << "PartSearch: " << (part_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_part << " microseconds)";
            logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;

            result_oss.str(""); // 清空字符串流
            result_oss << "PLLSearch: " << (pll_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_pll << " microseconds)";
            logFile << "[" << getCurrentTimestamp() << "] " << result_oss.str() << endl;


            stringstream match_oss;
            match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
            logFile << "[" << getCurrentTimestamp() << "] " << match_oss.str() << endl;

            EXPECT_EQ(bfs_result, part_result);
            EXPECT_EQ(bfs_result, pll_result);
        }

        // 计算并记录平均耗时
        stringstream final_oss;
        final_oss << "Final Average BiBFS Time: " << (query_count > 0 ? (total_duration_bfs / query_count) : 0) << " microseconds";
        logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;

        final_oss.str(""); // 清空字符串流
        final_oss << "Final Average PartSearch Time: " << (query_count > 0 ? (total_duration_part / query_count) : 0) << " microseconds";
        logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;

        final_oss.str(""); // 清空字符串流
        final_oss << "Final Average PLLSearch Time: " << (query_count > 0 ? (total_duration_pll / query_count) : 0) << " microseconds";
        logFile << "[" << getCurrentTimestamp() << "] " << final_oss.str() << endl;
    }

    // 关闭日志文件
    logFile.close();
}

//历史记录
TEST_F(ReachabilityTest, DISABLED_MultiPartitionTest)
{
    string edgeFile2 = PROJECT_ROOT_DIR"/Edges/medium/cit-DBLP";
    Graph g2(true);
    InputHandler inputHandler2(edgeFile2);
    inputHandler2.readGraph(g2);
    g2.setFilename(edgeFile2);
    // 初始化 CompressedSearch 并进行离线处理
    CompressedSearch comps2(g2, "Louvain");

    comps2.offline_industry(300,0.9,nullptr);
    //跨分区DFS查询改完了
    EXPECT_TRUE(comps2.reachability_query(6911,1631));
    EXPECT_TRUE(comps2.reachability_query(4137,12191));
    EXPECT_TRUE(comps2.reachability_query(1626,2433));
    EXPECT_TRUE(comps2.reachability_query(1661,733));
    EXPECT_TRUE(comps2.reachability_query(1626,9102));
    EXPECT_TRUE(comps2.reachability_query(1583,401));
    EXPECT_TRUE(comps2.reachability_query(1583,6216));
    EXPECT_TRUE(comps2.reachability_query(9353,7538));
    EXPECT_TRUE(comps2.reachability_query(6135,656));
    //这三个还有错误
    //同分区查询错误()
    EXPECT_TRUE(comps2.reachability_query(11388,2877));
    //同分区查询错误
    EXPECT_TRUE(comps2.reachability_query(3673,2772));
    //同分区查询错误
    EXPECT_TRUE(comps2.reachability_query(3076,5657));


}

TEST_F(ReachabilityTest, DISABLED_PartitionIndexTest)
{
    // 打开日志文件
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/"+getCurrentDaystamp()+"/PartitionIndextest_log.txt";

    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }

    // 遍历每个边文件
    for (const auto& edgeFilePath : edgeFiles) {
        cout << "Processing edge file: " << edgeFilePath << endl;
        logFile << "**************************************" << endl;
        logFile << "***********************************" << endl;
        logFile << "******************************" << endl;
        logFile << "*************************" << endl;
        logFile << "*****************" << endl;
        logFile << "*******" << endl;
        logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFilePath << endl;

        // 初始化图
        Graph g(true);  // 确保存储边集

        // 读取边文件
        InputHandler inputHandler(edgeFilePath);
        inputHandler.readGraph(g);
        g.setFilename(edgeFilePath);

        // 初始化 CompressedSearch 并进行离线处理
        CompressedSearch comps(g);
        comps.offline_industry(100,0.9,nullptr);

        // 获取分区管理器
        auto &pm = comps.get_partition_manager();
        auto reach_ratios = compute_reach_ratios(pm);

        // 输出分区信息
        logFile << "Partition Information:" << endl;
        for (const auto& partition : pm.get_mapping()) {
            int partition_id = partition.first;
            const auto& vertices = partition.second;
            float partition_ratio = reach_ratios.at(partition_id);
            logFile << "Partition " << partition_id << ": ";
            logFile << "Vertices: ";
            for (const auto& vertex : vertices) {
                logFile << vertex << " ";
            }
            logFile << ", Reach Ratio: " << partition_ratio << endl;
        }

    }

    // 关闭日志文件
    logFile.close();
}

TEST_F(ReachabilityTest, DISABLED_CompressedSearchPartitionInfoTest) {

    string outputFilePath = "partition_reach_ratio_results.csv";  // 输出文件路径



    // 打开输出文件
    ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        cout << "无法打开输出文件: " << outputFilePath << endl;
        return;
    }

    // 写入CSV头
    outputFile << "File,TotalReachRatio,PartitionID,PartitionReachRatio\n";

    // 进行 CompressedSearch 并输出分区信息
    for (const auto& edgeFilePath : edgeFiles) {
        cout << "正在处理: " << edgeFilePath << endl;

        // 初始化图
        Graph g(true);  // 确保存储边集

        // 读取边文件
        InputHandler inputHandler(edgeFilePath);
        inputHandler.readGraph(g);

        // 初始化 CompressedSearch 并进行离线处理
        CompressedSearch comps(g);
        comps.offline_industry();
        cout << "完成离线索引构建" << endl;

        // 获取分区管理器并输出分区信息
        PartitionManager pm = comps.get_partition_manager();
        // OutputHandler::printPartitionInfo(pm); // 如果需要，可以取消注释

        // 遍历每个分区并计算可达性比例


        // 如果没有分区，仍然记录全图的可达性比例
        if (pm.get_mapping().empty()) {
            outputFile << "\"" << edgeFilePath << "\"," << "N/A" << ",N/A,N/A\n";
            outputFile.flush();  // 确保立即写入文件
        }
    }

    // 关闭输出文件
    outputFile.close();

    cout << "Compressed search partition info testing completed. Results saved to " << outputFilePath << endl;
}

TEST_F(ReachabilityTest, DISABLED_LongDistanceTest) {

    // 打开日志文件
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/"+getCurrentDaystamp()+"/LongDistanceTest_log.txt";
    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }


    string edgeFile = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist";
    string edgefileDAG = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist_DAG";
    string mapping = PROJECT_ROOT_DIR"/Edges/DAGmapping/WikiTalk_edgelist_mapping_mapping";

    cout << "Processing edge file: " << edgeFile << endl;
    // 写入初始日志
    logFile << "**************************************" << endl;
    logFile << "*************************************" << endl;
    logFile << "************************************" << endl;
    logFile << "********************************" << endl;
    logFile << "*************************" << endl;
    logFile << "**************" << endl;
    logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFile << endl;

    // 初始化图
    Graph g(true);  // 确保存储边集
    Graph dag(true);
    // 读取边文件
    InputHandler inputHandler(edgeFile);
    inputHandler.readGraph(g);
    g.setFilename(edgeFile);
    InputHandler inputHandlerDAG(edgefileDAG);
    inputHandlerDAG.readGraph(dag);
    dag.setFilename(edgefileDAG);

    // 初始化 BFS
    BidirectionalBFS bfs(g);

    // 初始化 PLL 并进行离线处理    
    PLL pll(g);
    pll.offline_industry();
    logFile << "[" << getCurrentTimestamp() << "] " << "完成 PLL 离线索引构建" << endl;

    // 获取 PLL 索引大小并记录
    auto pllIndexSizes = pll.getIndexSizes();
    stringstream oss;
    oss << "PLL Index Sizes - IN: " << pllIndexSizes[3].second << ", OUT: " << pllIndexSizes[4].second;
    logFile << "[" << getCurrentTimestamp() << "] " << oss.str() << endl;


    // 初始化 CompressedSearch 并进行离线处理
    CompressedSearch comps(dag,"ReachRatio");
    comps.offline_industry(200,0.3,mapping);
    logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;

    logFile << "Partition mapping:" << std::endl;
    auto &pm = comps.get_partition_manager();
    for (const auto& [partition, nodes] : pm.get_mapping()) {
        if(partition == -1||nodes.size()<1) continue;
        logFile << "Partition " << partition << ": ";
        for (int node : nodes) {
            logFile << node << " ";
        }
        logFile << std::endl;
    }


    logFile <<"     PLL index size: "<<endl;
    auto indices =  pll.getIndexSizes();
    for(auto &index: indices){
        logFile << "[" << getCurrentTimestamp() << "] " << index.first << ": " << index.second << endl;
    }
    logFile <<"     Comps index size: "<<endl;
    indices = comps.getIndexSizes();
    for(auto &index: indices){
        logFile << "[" << getCurrentTimestamp() << "] " << index.first << ": " << index.second << endl;
    }

    string queryFile = PROJECT_ROOT_DIR"/Edges/DAGs/large/WikiTalk_edgelist_DAG_query.txt";

    // 读取查询点对
    vector<pair<int, int>> query_pairs_6 = readQueryPairs(queryFile, 6, 100);
    vector<pair<int, int>> query_pairs_8 = readQueryPairs(queryFile, 8, 100);
    vector<pair<int, int>> query_pairs_10 = readQueryPairs(queryFile, 10, 100);



    logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;
    auto lines = comps.get_index_info();
    for(auto line : lines){
        logFile<<line<<endl;
    }

    // 查询并比较时间
    auto query_and_log = [&](const vector<pair<int, int>>& query_pairs, int distance) {
        long long total_duration_bfs = 0;
        long long total_duration_compressed = 0;
        int query_count = 0;

        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"****************************************************"<<endl;
        logFile<<"开始查询距离为 "<<distance<<"的可达点对"<<endl;

        for (const auto& query_pair : query_pairs) {
            int source = query_pair.first;
            int target = query_pair.second;
            cout << "[" << getCurrentTimestamp() << "] " << "Querying from " << source << " to " << target << endl;
            // 测量 BiBFS 查询耗时
            auto start_bfs = chrono::high_resolution_clock::now();
            bool bfs_result = bfs.reachability_query(source, target);
            auto end_bfs = chrono::high_resolution_clock::now();
            auto duration_bfs = chrono::duration_cast<chrono::microseconds>(end_bfs - start_bfs).count();

            // 测量 CompressedSearch 查询耗时
            auto start_compressed = chrono::high_resolution_clock::now();
            bool compressed_result = comps.reachability_query(source, target);
            auto end_compressed = chrono::high_resolution_clock::now();
            auto duration_compressed = chrono::duration_cast<chrono::microseconds>(end_compressed - start_compressed).count();

            // 测量 PLL 查询耗时
            auto start_pll = chrono::high_resolution_clock::now();
            bool pll_result = pll.reachability_query(source, target);
            auto end_pll = chrono::high_resolution_clock::now();
            auto duration_pll = chrono::duration_cast<chrono::microseconds>(end_compressed - start_compressed).count();


            query_count++;
            total_duration_bfs += duration_bfs;
            total_duration_compressed += duration_compressed;
            auto results_match = (bfs_result == compressed_result) && (bfs_result == pll_result); //if (!results_match) continue;
            EXPECT_EQ(bfs_result, compressed_result);
            EXPECT_EQ(bfs_result, pll_result);

            // 输出查询结果和耗时
            logFile << "Distance " << distance << " Query from " << source << " to " << target << endl;
            logFile << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)" << endl;
            logFile << "CompressedSearch: " << (compressed_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_compressed << " microseconds)" << endl;
            logFile << "PLL: " << (pll_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_pll << " microseconds)" << endl;
            stringstream match_oss;
            match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
            logFile << "[" << getCurrentTimestamp() << "] " << match_oss.str() << endl;

        }

        // 计算并记录平均耗时
        logFile << "Distance " << distance << " Final Average BiBFS Time: " << (query_count > 0 ? (total_duration_bfs / query_count) : 0) << " microseconds" << endl;
        logFile << "Distance " << distance << " Final Average CompressedSearch Time: " << (query_count > 0 ? (total_duration_compressed / query_count) : 0) << " microseconds" << endl;
    };

    // 对每个距离进行查询并记录结果
    query_and_log(query_pairs_6, 6);
    query_and_log(query_pairs_8, 8);
    query_and_log(query_pairs_10, 10);

    // 关闭日志文件
    logFile.close();
}

TEST_F(ReachabilityTest, SingleTest){
    string edgeFile = PROJECT_ROOT_DIR"/Edges/medium/moreno_health_unweighted";
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/" + getCurrentDaystamp() + "/singletest.txt";
    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }
    // 写入初始日志
    logFile << "**************************************" << endl;
    logFile << "*************************************" << endl;
    logFile << "************************************" << endl;
    logFile << "********************************" << endl;
    logFile << "*************************" << endl;
    logFile << "**************" << endl;
    logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFile << endl;

    // 初始化图
    Graph g(true);  // 确保存储边集
    // 读取边文件
    InputHandler inputHandler(edgeFile);
    inputHandler.readGraph(g);
    g.setFilename(edgeFile);


    // 生成查询对
    int num_queries = 1000;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    //改成允许重复的 query
    vector<pair<int, int>> query_pairs = RandomUtils::generateQueryPairs(num_queries, max_value, seed);
    
    // 初始化 BFS
    BidirectionalBFS bfs(g);
    // 初始化 CompressedSearch 并进行离线处理
    CompressedSearch comps(g, "Random");
    comps.offline_industry(200, 0.3, "");
    logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;


    // 查询并比较时间
    auto query_and_log = [&](const vector<pair<int, int>>& query_pairs) {
        long long total_duration_bfs = 0;
        long long total_duration_compressed = 0;
        int query_count = 0;

        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;

        for (const auto& query_pair : query_pairs) {
            int source = query_pair.first;
            int target = query_pair.second;
            cout << "[" << getCurrentTimestamp() << "] " << "Querying from " << source << " to " << target << endl;
            // 测量 BiBFS 查询耗时
            auto start_bfs = chrono::high_resolution_clock::now();
            bool bfs_result = bfs.reachability_query(source, target);
            auto end_bfs = chrono::high_resolution_clock::now();
            auto duration_bfs = chrono::duration_cast<chrono::microseconds>(end_bfs - start_bfs).count();

            // 测量 CompressedSearch 查询耗时
            auto start_compressed = chrono::high_resolution_clock::now();
            bool compressed_result = comps.reachability_query(source, target);
            auto end_compressed = chrono::high_resolution_clock::now();
            auto duration_compressed = chrono::duration_cast<chrono::microseconds>(end_compressed - start_compressed).count();

            query_count++;
            total_duration_bfs += duration_bfs;
            total_duration_compressed += duration_compressed;
            auto results_match = (bfs_result == compressed_result);//if(!results_match) continue;
  
            EXPECT_EQ(bfs_result, compressed_result);

            // 输出查询结果和耗时
            logFile << " Query from " << source << " to " << target << std::endl;
            logFile << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)" << std::endl;
            logFile << "CompressedSearch: " << (compressed_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_compressed << " microseconds)" << std::endl;
            stringstream match_oss;
            match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
            logFile << "[" << getCurrentTimestamp() << "] " << match_oss.str() << std::endl;
        }

        // 计算并记录平均耗时
        logFile << " Final Average BiBFS Time: " << (query_count > 0 ? (total_duration_bfs / query_count) : 0) << " microseconds" << std::endl;
        logFile << " Final Average CompressedSearch Time: " << (query_count > 0 ? (total_duration_compressed / query_count) : 0) << " microseconds" << std::endl;
        
    };
    query_and_log(query_pairs);

    // 关闭日志文件
    logFile.close();

}


TEST_F(ReachabilityTest, DISABLED_BasicTest) {
    
    // string edgeFile = PROJECT_ROOT_DIR "/Edges/large/tweibo-edgelist";
    // string edgefileDAG = PROJECT_ROOT_DIR "/Edges/DAGs/large/tweibo-edgelist_DAG";
    // string mapping = PROJECT_ROOT_DIR "/Edges/DAGmapping/tweibo-edgelist_mapping";
    // string queryFile = PROJECT_ROOT_DIR "/QueryPairs/tweibo-edgelist_DAG_distance_pairs.txt";
    // string edgeFile = PROJECT_ROOT_DIR "/Edges/large/dbpedia_edgelist";
    // string edgefileDAG = PROJECT_ROOT_DIR "/Edges/DAGs/large/dbpedia_edgelist_DAG";
    // string mapping = PROJECT_ROOT_DIR "/Edges/DAGmapping/dbpedia_edgelist_mapping";
    // string edgeFile = PROJECT_ROOT_DIR "/Edges/large/LiveJournal";
    // string edgefileDAG = PROJECT_ROOT_DIR "/Edges/DAGs/large/LiveJournal_DAG";
    // string mapping = PROJECT_ROOT_DIR "/Edges/DAGmapping/LiveJournal_mapping";
    // string queryFile = PROJECT_ROOT_DIR "/QueryPairs/tweibo-edgelist_DAG_distance_pairs.txt";

    string edgeFile = PROJECT_ROOT_DIR "/Edges/medium/cit-DBLP";
    string edgefileDAG = PROJECT_ROOT_DIR "/Edges/DAGs/medium/cit-DBLP_DAG";
    string mapping = PROJECT_ROOT_DIR "/Edges/DAGmapping/cit-DBLP_mapping";
    // 打开日志文件
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/" + getCurrentDaystamp() + "/Basictest_LiveJournal_log.txt";
    // string logFilePath = string(PROJECT_ROOT_DIR) + "/result/" + getCurrentDaystamp() + "/Basictest_dbpedia_log.txt";
    // string logFilePath = string(PROJECT_ROOT_DIR) + "/result/" + getCurrentDaystamp() + "/Basictest_tweibo_log.txt";
    
    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }

    // 读取查询点对
    // vector<pair<int, int>> query_pairs_6 = readQueryPairs(queryFile, 6, 100);
    // vector<pair<int, int>> query_pairs_8 = readQueryPairs(queryFile, 8, 100);
    // vector<pair<int, int>> query_pairs_10 = readQueryPairs(queryFile, 10, 100);

    cout << "Processing edge file: " << edgeFile << endl;
    // 写入初始日志
    logFile << "**************************************" << endl;
    logFile << "*************************************" << endl;
    logFile << "************************************" << endl;
    logFile << "********************************" << endl;
    logFile << "*************************" << endl;
    logFile << "**************" << endl;
    logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFile << endl;


    stringstream oss;

    // 初始化图
    Graph g(true);  // 确保存储边集
    Graph dag(true);
    // 读取边文件
    InputHandler inputHandler(edgeFile);
    inputHandler.readGraph(g);
    g.setFilename(edgeFile);
    InputHandler inputHandlerDAG(edgefileDAG);
    inputHandlerDAG.readGraph(dag);
    dag.setFilename(edgefileDAG);

    // 生成查询对
    int num_queries = 1000;
    int max_value = g.vertices.size();
    unsigned int seed = 42; // 可选的随机种子

    //改成允许重复的 query
    vector<pair<int, int>> query_pairs = RandomUtils::generateQueryPairs(num_queries, max_value, seed);
    


    // 初始化 BFS
    BidirectionalBFS bfs(g);
    // 初始化 CompressedSearch 并进行离线处理
    CompressedSearch comps(dag, "MultiCut");
    comps.offline_industry(200, 0.3, mapping);
    logFile << "[" << getCurrentTimestamp() << "] " << "完成 Compress 离线索引构建" << endl;


    logFile << "Partition mapping:" << std::endl;
    auto &pm = comps.get_partition_manager();
    for (const auto& [partition, nodes] : pm.get_mapping()) {
        if (partition == -1 || nodes.size() < 1) continue;
        logFile << "Partition " << partition << ": ";
        for (int node : nodes) {
            logFile << node << " ";
        }
        logFile << std::endl;
    }

    logFile << "Comps index size: " << std::endl;
    auto indices = comps.getIndexSizes();
    for (auto &index : indices) {
        logFile << "[" << getCurrentTimestamp() << "] " << index.first << ": " << index.second << std::endl;
    }


    // 查询并比较时间
    auto query_and_log = [&](const vector<pair<int, int>>& query_pairs, int distance) {
        long long total_duration_bfs = 0;
        long long total_duration_compressed = 0;
        int query_count = 0;

        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "开始查询距离为 " << distance << " 的可达点对" << std::endl;

        for (const auto& query_pair : query_pairs) {
            int source = query_pair.first;
            int target = query_pair.second;
            cout << "[" << getCurrentTimestamp() << "] " << "Querying from " << source << " to " << target << endl;
            // 测量 BiBFS 查询耗时
            auto start_bfs = chrono::high_resolution_clock::now();
            bool bfs_result = bfs.reachability_query(source, target);
            auto end_bfs = chrono::high_resolution_clock::now();
            auto duration_bfs = chrono::duration_cast<chrono::microseconds>(end_bfs - start_bfs).count();

            // 测量 CompressedSearch 查询耗时
            auto start_compressed = chrono::high_resolution_clock::now();
            bool compressed_result = comps.reachability_query(source, target);
            auto end_compressed = chrono::high_resolution_clock::now();
            auto duration_compressed = chrono::duration_cast<chrono::microseconds>(end_compressed - start_compressed).count();

            query_count++;
            total_duration_bfs += duration_bfs;
            total_duration_compressed += duration_compressed;
            auto results_match = (bfs_result == compressed_result);if(!results_match) continue;
  
            EXPECT_EQ(bfs_result, compressed_result);

            // 输出查询结果和耗时
            logFile << "Distance " << distance << " Query from " << source << " to " << target << std::endl;
            logFile << "BiBFS: " << (bfs_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_bfs << " microseconds)" << std::endl;
            logFile << "CompressedSearch: " << (compressed_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_compressed << " microseconds)" << std::endl;
            stringstream match_oss;
            match_oss << "***** Does Result Match from " << source << " query to " << target << ": " << (results_match ? "Match" : "Not Match") << " *****";
            logFile << "[" << getCurrentTimestamp() << "] " << match_oss.str() << std::endl;
        }

        // 计算并记录平均耗时
        logFile << "Distance " << distance << " Final Average BiBFS Time: " << (query_count > 0 ? (total_duration_bfs / query_count) : 0) << " microseconds" << std::endl;
        logFile << "Distance " << distance << " Final Average CompressedSearch Time: " << (query_count > 0 ? (total_duration_compressed / query_count) : 0) << " microseconds" << std::endl;
        
    };

    // 对每个距离进行查询并记录结果
    // query_and_log(query_pairs_6, 6);
    // query_and_log(query_pairs_8, 8);
    // query_and_log(query_pairs_10, 10);

    query_and_log(query_pairs, 0);

    // 关闭日志文件
    logFile.close();
}