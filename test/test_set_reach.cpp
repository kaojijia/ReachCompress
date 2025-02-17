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
#include "SetSearch.h"
#include <dirent.h>      // 用于目录遍历
#include <sys/types.h>   // 定义了DIR等数据类型
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

class SetReachabilityTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // 初始化代码
            // 获取所有边文件
            // string edgesDirectory = string(PROJECT_ROOT_DIR) + "/Edges/test";
    
            // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/ia-radoslaw-email_edges.txt");
            edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/DAGs/large/WikiTalk_DAG");
            // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/DAGs/medium/cit-DBLP_DAG");
            // edgeFiles.push_back(string(PROJECT_ROOT_DIR) + "/Edges/DAGs/test/in2004_DAG");
            // edgeFiles.push_back("/root/Projects/ReachCompress/Edges/DAGs/medium/soc-epinions_DAG");
    
            // edgeFiles = this->getAllFiles(edgesDirectory);
            if (edgeFiles[0].empty()) {
                FAIL() << "没有找到任何边文件。";
            }
        }
    
        void TearDown() override {
            // 清理代码
        }
    
        std::vector<int> generateRandomVector(int num, int range) {
            std::vector<int> vec(num);
            std::random_device rd;
            std::mt19937 gen(rd());
            // 生成 [1, 10000] 范围内的随机整数
            std::uniform_int_distribution<int> dis(1, range);
            std::generate(vec.begin(), vec.end(), [&]() { return dis(gen); });
            return vec;
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
            return "[" + ss.str()+"]";
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
    
    
    
        vector<string> edgeFiles;
    };

TEST_F(SetReachabilityTest, basic_test){
    Graph g;
    // vector<pair<int, int>> edges = { {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}};
    // edges.push_back(make_pair(4, 11));
    // edges.push_back(make_pair(4,12));
    // edges.push_back(make_pair(13,4));
    // edges.push_back(make_pair(1,13));
    // edges.push_back(make_pair(7,15));
    // edges.push_back(make_pair(7,16));
    // edges.push_back(make_pair(17,7));

    // for(auto edge : edges){
    //     g.addEdge(edge.first, edge.second);
    // }
    
    
    //读图，新建日志文件
    InputHandler input_handler(edgeFiles[0]);
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/"+getCurrentDaystamp()+"/SetReachability_log.txt";
    ofstream logFile(logFilePath, ios::app);
    if (!logFile.is_open()) {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }
    logFile<<"====================================="<<endl;
    logFile<<"====================================="<<endl;
    logFile<<"====================================="<<endl;
    logFile<<"====================================="<<endl;
    logFile<<getCurrentTimestamp()<<" 读图开始，当前路径为"<<edgeFiles[0]<<endl;
    input_handler.readGraph(g);

    if(g.hasCycle()){
        cout<<"图中存在环，无法进行后续操作。"<<endl;
        logFile << "图中存在环，无法进行后续操作。" << endl;
        logFile.close();
        return;
    }
    logFile<<getCurrentTimestamp()<<" 读图完成，开始构建索引"<<endl;
    //构建索引
    SetSearch set_search(g);
    set_search.offline_industry();
    logFile<<getCurrentTimestamp()<<" 构建索引完成，生成随机查询集"<<endl;

    // EXPECT_TRUE(set_search.reachability_query(1,15));

    // vector<int> source_set = {1, 3, 5, 121};
    // vector<int> target_set = {7, 8,123,1, 122};

    //产生随机查询集
    vector<int> source_set = generateRandomVector(10000, g.vertices.size());
    vector<int> target_set = generateRandomVector(10000, g.vertices.size());
    logFile<<getCurrentTimestamp()<<" 随机查询集生成完成，开始测试"<<endl;
    //set_reach 测试
    auto start = std::chrono::high_resolution_clock::now();
    auto result = set_search.set_reachability_query(source_set, target_set);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    logFile<<getCurrentTimestamp() << " set_reachability_query 用时 " << duration << " 微秒" << std::endl;

    //pll 测试
    vector<pair<int,int>> query_pairs;
    for(auto s:source_set){
        for(auto t:target_set){
            query_pairs.push_back(make_pair(s,t));
        }
    }    
    std::random_device rd;
    std::mt19937 r(rd());
    std::shuffle(query_pairs.begin(), query_pairs.end(), r);

    int count = 0;
    bool flag = false;
    vector<pair<int,int>> result_pll(query_pairs.size()/2);
    start = std::chrono::high_resolution_clock::now();
    for(auto [s,t]:query_pairs){
        flag = set_search.pll->reachability_query(s,t);
        if(flag){
            count++;
            result_pll.emplace_back(s,t);
        }
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "pll_reachability_query 用时 " << duration << " 微秒" << std::endl;
    logFile <<getCurrentTimestamp()<<" pll_reachability_query 用时 " << duration << " 微秒" << std::endl;

    //对result_pll去重, 并且去掉两个元素相同的pair
    vector<pair<int, int>> temp;
    for(auto p : result_pll){
        if(p.first != p.second){
            temp.push_back(p);
        }
    }
    result_pll.clear();
    result_pll = temp;
    sort( result_pll.begin(), result_pll.end() );
    result_pll.erase( unique( result_pll.begin(), result_pll.end() ), result_pll.end() );
    
    // result_set去重处理
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());
    
    EXPECT_EQ(result_pll.size(), result.size());
    logFile <<getCurrentTimestamp()<< " result_pll可达点对数量 = " << result_pll.size() << endl;
    logFile <<getCurrentTimestamp()<< " result_set可达点对数量 = " << result.size() << endl;
    logFile.close();
    return;
}