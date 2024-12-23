#include "gtest/gtest.h"
#include "pll.h"
#include "graph.h"
#include "BidirectionalBFS.h"
#include "utils/OutputHandler.h"
#include "utils/RandomUtils.h"
#include "utils/InputHandler.h"
#include <string>

vector<pair<int, int>> readQueryPairs(const string &filePath, int distance, int num_pairs)
{
    vector<pair<int, int>> query_pairs;
    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "无法打开文件: " << filePath << endl;
        return query_pairs;
    }

    string line;
    bool in_distance_section = false;
    while (getline(file, line))
    {
        if (line == "Distance " + to_string(distance) + ":")
        {
            in_distance_section = true;
            continue;
        }
        if (in_distance_section)
        {
            if (line.empty())
            {
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
    if (query_pairs.size() > num_pairs)
    {
        random_device rd;
        mt19937 g(rd());
        shuffle(query_pairs.begin(), query_pairs.end(), g);
        query_pairs.resize(num_pairs);
    }

    return query_pairs;
}

string getCurrentTimestamp()
{
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

string getCurrentDaystamp()
{
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

TEST(PLLTest, DISABLED_BIGVertexTest)
{
    Graph g(true);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 4);
    g.addEdge(4, 100);
    PLL pll(g);
    pll.offline_industry();
    pll.getCurrentTimestamp();
    EXPECT_TRUE(pll.reachability_query(1, 4));
    EXPECT_TRUE(pll.reachability_query(1, 100));
    EXPECT_FALSE(pll.reachability_query(1, 5));
    EXPECT_FALSE(pll.reachability_query(1, 6));
    EXPECT_FALSE(pll.reachability_query(123, 68));
    EXPECT_FALSE(pll.reachability_query(43, 1));
    pll.getCurrentTimestamp();
}
TEST(PLLTest, DISABLED_ReachabilityTest)
{
    Graph g1(true);
    Graph g2(true);

    // 使用项目根目录宏构建绝对路径
    std::string filename = "Edges/DAGs/large/tweibo-edgelist_DAG";
    // std::string gene_path = "/Edges/generate/";
    std::string result_path = "/result/";

    // InputHandler inputHandler1(PROJECT_ROOT_DIR + gene_path + filename);
    // inputHandler1.readGraph(g1);
    // OutputHandler outputHandler_origin(PROJECT_ROOT_DIR + result_path+ filename + "_origin");

    // OutputHandler::printGraphInfo(g1);

    InputHandler inputHandler2(PROJECT_ROOT_DIR + filename);
    inputHandler2.readGraph(g2);
    OutputHandler outputHandler_pruned(PROJECT_ROOT_DIR + result_path + filename + "_pruned");

    // PLL pll_origin(g1);
    // pll_origin.buildPLLLabelsUnpruned();
    // outputHandler_origin.writeInOutSets(g1);

    PLL pll2(g2);
    pll2.buildPLLLabels();
    outputHandler_pruned.writeInOutSets(pll2);

    // 生成不重复的随机查询对
    int num_queries = 100;
    int max_value = g2.vertices.size();
    unsigned int seed = 42; // 可选的随机种子
    std::vector<std::pair<int, int>> query_pairs = RandomUtils::generateUniqueQueryPairs(num_queries, max_value, seed);

    cout << pll2.getCurrentTimestamp() << endl;
    for (const auto &query_pair : query_pairs)
    {
        int source = query_pair.first;
        int target = query_pair.second;
        bool result = pll2.query(source, target);
        std::cout << "Query from " << source << " to " << target << ": " << (result ? "Reachable" : "Not Reachable") << std::endl;
    }
    cout << pll2.getCurrentTimestamp() << endl;
}

TEST(PLLTest, ReachabilityTest_MT)
{
    string logFilePath = string(PROJECT_ROOT_DIR) + "/result/" + getCurrentDaystamp() + "/PLL_Test_log.txt";
    ofstream logFile(logFilePath, ios::out);
    if (!logFile.is_open())
    {
        FAIL() << "无法打开日志文件: " << logFilePath;
    }

    string edgeFile = PROJECT_ROOT_DIR "/Edges/large/tweibo-edgelist";
    string edgefileDAG = PROJECT_ROOT_DIR "/Edges/DAGs/large/WikiTalk_edgelist_DAG";
    string mapping = PROJECT_ROOT_DIR "/Edges/DAGmapping/tweibo-edgelist_mapping";
    string queryFile = PROJECT_ROOT_DIR "/QueryPairs/tweibo-edgelist_DAG_distance_pairs.txt";

    // 读取查询点对
    vector<pair<int, int>> query_pairs_6 = readQueryPairs(queryFile, 6, 100);
    vector<pair<int, int>> query_pairs_8 = readQueryPairs(queryFile, 8, 100);
    vector<pair<int, int>> query_pairs_10 = readQueryPairs(queryFile, 10, 100);

    cout << "Processing edge file for PLL: " << edgeFile << endl;
    // 写入初始日志
    logFile << "**************************************" << endl;
    logFile << "[" << getCurrentTimestamp() << "] " << "当前处理文件: " << edgeFile << endl;

    // 初始化图
    Graph g(true); // 确保存储边集
    // 读取边文件
    InputHandler inputHandler(edgeFile);
    inputHandler.readGraph(g);
    g.setFilename(edgeFile);

    // 初始化 PLL 并进行离线处理
    PLL pll(g);
    pll.offline_industry();
    logFile << "[" << getCurrentTimestamp() << "] " << "完成 PLL 离线索引构建" << endl;

    // 获取 PLL 索引大小并记录
    auto pllIndexSizes = pll.getIndexSizes();
    stringstream oss;
    for(const auto &[indexName, indexSize] : pllIndexSizes)
    {
        oss << indexName << ": " << indexSize << " bytes";
        logFile << "[" << getCurrentTimestamp() << "] " << oss.str() << endl;
    }


    // 计算每个方法的平均耗时
    long long total_duration_pll = 0;
    int query_count = 0;

    // 查询并比较时间
    auto query_and_log = [&](const vector<pair<int, int>> &query_pairs, int distance)
    {
        long long total_duration_bfs = 0;
        long long total_duration_compressed = 0;
        long long total_duration_pll = 0;
        int query_count = 0;

        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "****************************************************" << std::endl;
        logFile << "开始查询距离为 " << distance << " 的可达点对" << std::endl;

        for (const auto &query_pair : query_pairs)
        {
            int source = query_pair.first;
            int target = query_pair.second;
            cout << "[" << getCurrentTimestamp() << "] " << "Querying from " << source << " to " << target << endl;

            // 测量 PLL 查询耗时
            auto start_pll = chrono::high_resolution_clock::now();
            bool pll_result = pll.reachability_query(source, target);
            auto end_pll = chrono::high_resolution_clock::now();
            auto duration_pll = chrono::duration_cast<chrono::microseconds>(end_pll - start_pll).count();

            query_count++;
            total_duration_pll += duration_pll;

            // 输出查询结果和耗时
            logFile << "Distance " << distance << " Query from " << source << " to " << target << std::endl;
            logFile << "PLL: " << (pll_result ? "Reachable" : "Not Reachable") << " (Time: " << duration_pll << " microseconds)" << std::endl;
        }

        logFile << "Distance " << distance << " Final Average PLL Time: " << (query_count > 0 ? (total_duration_pll / query_count) : 0) << " microseconds" << std::endl;
    };

    // 对每个距离进行查询并记录结果
    query_and_log(query_pairs_6, 6);
    query_and_log(query_pairs_8, 8);
    query_and_log(query_pairs_10, 10);

    // 关闭日志文件
    logFile.close();
}

TEST(PLLTest, DISABLED_IndexSizeTest)
{
    std::string filename = "/Edges/DAGs/large/tweibo-edgelist_DAG";
    Graph g(true);
    InputHandler inputHandler(PROJECT_ROOT_DIR + filename);
    inputHandler.readGraph(g);
    PLL pll(g);
    pll.buildPLLLabels();
    std::unordered_map<std::string, size_t> indexSizes = pll.getIndexSizes();
    for (const auto &[indexName, indexSize] : indexSizes)
    {
        std::cout << indexName << ": " << indexSize << " bytes" << std::endl;
    }
}