#include "Hypergraph.h"
#include "HypergraphTreeIndex.h" // Include the new header
#include "UndirectedPLL.h"
#include "UWeightedPLL.h" // Correct include for WeightedPrunedLandmarkIndex
#include <gtest/gtest.h>
#include <fstream>
#include "SimplexReader.h"
#include <iostream> // For std::cout in test
#include <chrono>   // For timing
#include <random>   // For random number generation
#include <vector>
#include <iomanip> // For std::setw, std::fixed, std::setprecision
#include <thread>  // Include thread header
#include <atomic>  // Potentially useful for shared flags/counters if needed
#include <map>     // For storing generated queries by type

using namespace std;
using namespace std::chrono;

class HypergraphTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // hg.addVertices(3);
        // edge1 = {0, 1};
        // edge2 = {1, 2};
    }

    // Hypergraph hg;
    // vector<int> edge1;
    // vector<int> edge2;
};

TEST_F(HypergraphTest, BasicOperations)
{
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg;
    vector<int> edge1;
    vector<int> edge2;
    hg.addVertices(3);
    edge1 = {0, 1};
    edge2 = {1, 2};
    EXPECT_EQ(hg.numVertices(), 3);

    EXPECT_EQ(hg.addHyperedge(edge1), 0);
    EXPECT_EQ(hg.addHyperedge(edge2), 1);
    EXPECT_EQ(hg.numHyperedges(), 2);

    auto &edges0 = hg.getIncidentHyperedges(0);
    ASSERT_EQ(edges0.size(), 1);
    EXPECT_EQ(edges0[0], 0);
}

TEST_F(HypergraphTest, ConnectedComponents)
{
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg;
    hg.addVertices(6);
    hg.addHyperedge({0, 1, 2});
    hg.addHyperedge({3, 4, 5});

    auto components = hg.getConnectedComponents();
    ASSERT_EQ(components.size(), 2);
    EXPECT_EQ(components[0].size(), 3);
    EXPECT_EQ(components[1].size(), 3);
}

TEST_F(HypergraphTest, Reachability)
{
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg(0, 0);
    // 添加15个顶点
    auto num = hg.addVertices(15);
    EXPECT_EQ(num, 15);

    // 连通边的链 {0,1,2}-{2,3}-{4,5,6}
    hg.addHyperedge({0, 1, 2, 3});
    hg.addHyperedge({2, 3, 4});
    hg.addHyperedge({4, 5, 6});
    hg.addHyperedge({14, 6});

    // 环状连通 {7,8,9}-{9,10}-{10,11,7}
    hg.addHyperedge({7, 8, 9, 12});
    hg.addHyperedge({9, 10, 12});
    hg.addHyperedge({10, 11, 7, 12});

    // 孤立顶点12-14（不添加任何边）

    hg.offline_industry();

    // 验证连通路径

    EXPECT_TRUE(hg.isReachable(2, 0));

    // 验证跨分量不可达
    EXPECT_FALSE(hg.isReachable(0, 7));
    EXPECT_FALSE(hg.isReachable(10, 2));

    // 验证环内可达
    EXPECT_TRUE(hg.isReachable(8, 11));

    // 验证孤立点自连通
    EXPECT_TRUE(hg.isReachable(13, 13));

    // 测试不存在的顶点
    EXPECT_THROW(hg.isReachable(0, 120), invalid_argument);
    EXPECT_THROW(hg.isReachable(97, 0), invalid_argument);

    // 验证在参数控制下的可达性
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0, 6, 2));
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0, 6, 3));
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0, 14, 2));
    EXPECT_TRUE(hg.isReachableBidirectionalBFS(0, 6, 1));

    EXPECT_FALSE(hg.isReachableBidirectionalBFS(8, 11, 3));
    EXPECT_TRUE(hg.isReachableBidirectionalBFS(8, 11, 2));

    // 验证算法正确性
    cout << endl;
    for (int i = 0; i < 15; ++i)
    {
        for (int j = 0; j < 15; ++j)
        {
            for (int k = 1; k <= 4; ++k)
            {
                bool bfsResult = hg.isReachableBidirectionalBFS(i, j, k);
                bool graphResult = hg.isReachableViaWeightedGraph(i, j, k);
                bool pllResult = hg.isReachableViaPLLWeightedGraph(i, j, k);
                EXPECT_EQ(bfsResult, graphResult);
                EXPECT_EQ(pllResult, graphResult);
            }
        }
    }
}

TEST_F(HypergraphTest, PLI)
{
    GTEST_SKIP() << "Test disabled.";
    // 1. 构建一个无向带权图 wg
    // 设定一个边权阈值，比如 5，忽略 weight<5 的边
    WeightedGraph wg(29 /*顶点数*/, 5 /*最小可通过的边权重*/);
    wg.addEdge(0, 1, 1);
    wg.addEdge(1, 2, 3);
    // wg.addEdge(2, 3, 10);
    wg.addEdge(3, 4, 2);
    wg.addEdge(2, 5, 7);
    // 链
    wg.addEdge(5, 6, 19);
    wg.addEdge(6, 7, 19);
    wg.addEdge(7, 8, 19);
    wg.addEdge(8, 9, 19);
    wg.addEdge(9, 10, 19);
    wg.addEdge(10, 20, 19);
    wg.addEdge(20, 21, 19);
    // 环
    wg.addEdge(11, 12, 19);
    wg.addEdge(12, 13, 19);
    wg.addEdge(13, 14, 19);
    wg.addEdge(14, 15, 19);
    wg.addEdge(15, 11, 19);

    // 2. 创建索引
    WeightedPrunedLandmarkIndex wp(wg);
    wp.offline_industry();

    // 这四个有问题，没有足够的记录
    EXPECT_TRUE(wp.reachability_query(2, 10, 5));
    EXPECT_TRUE(wp.reachability_query(5, 10, 5));
    EXPECT_TRUE(wp.reachability_query(10, 6, 5));
    EXPECT_TRUE(wp.reachability_query(5, 21, 5));

    EXPECT_TRUE(wp.reachability_query(11, 14, 5));
    EXPECT_FALSE(wp.reachability_query(0, 1, 5));
    EXPECT_FALSE(wp.reachability_query(2, 3, 5));
    // EXPECT_TRUE(wp.reachability_query(3, 5,5));
    EXPECT_FALSE(wp.reachability_query(0, 5, 5));
    EXPECT_FALSE(wp.reachability_query(0, 2, 5));
    EXPECT_FALSE(wp.reachability_query(0, 4, 5));
    EXPECT_FALSE(wp.reachability_query(1, 4, 5));

    // 3. 查询可达性
    // 链太长了找不到
    // EXPECT_TRUE(wg.landmark_reachability_query(2, 10));
    // EXPECT_TRUE(wg.landmark_reachability_query(5, 10));
    // EXPECT_TRUE(wg.landmark_reachability_query(10, 6));
    // EXPECT_TRUE(wg.landmark_reachability_query(5, 21));

    // EXPECT_TRUE(wg.landmark_reachability_query(11, 14));
    // EXPECT_FALSE(wg.landmark_reachability_query(0, 1));
    // EXPECT_TRUE(wg.landmark_reachability_query(2, 3));
    // EXPECT_TRUE(wg.landmark_reachability_query(3, 5));
    // EXPECT_FALSE(wg.landmark_reachability_query(0, 5));
    // EXPECT_FALSE(wg.landmark_reachability_query(0, 2));
    // EXPECT_FALSE(wg.landmark_reachability_query(0, 4));
    // EXPECT_FALSE(wg.landmark_reachability_query(1, 4));
}

TEST_F(HypergraphTest, FileLoading)
{
    GTEST_SKIP() << "Test disabled.";
    const string test_file = "test_hyper.txt";
    ofstream fout(test_file);
    fout << "0 1 2\n3 4\n5 6 7 8";
    fout.close();

    Hypergraph hg = Hypergraph::fromFile(test_file);
    EXPECT_EQ(hg.numVertices(), 9);
    EXPECT_EQ(hg.numHyperedges(), 3);

    auto &first_edge = hg.getHyperedge(0);
    ASSERT_EQ(first_edge.vertices.size(), 3);
    EXPECT_EQ(first_edge.vertices[0], 0);
}

TEST_F(HypergraphTest, ExceptionHandling)
{
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg(3);

    EXPECT_THROW(hg.addHyperedge({0, 5}), invalid_argument);
    EXPECT_THROW(hg.getHyperedge(10), invalid_argument);
}

TEST_F(HypergraphTest, SimplexToHypergraphConversion)
{
    GTEST_SKIP() << "Test disabled.";
    // 创建测试文件
    const string nverts_file = "test_nverts.txt";
    const string simplices_file = "test_simplices.txt";
    const string output_file = "test_hypergraph.txt";

    ofstream nverts_out(nverts_file);
    nverts_out << "3\n2\n4\n";
    nverts_out.close();

    ofstream simplices_out(simplices_file);
    simplices_out << "1\n2\n3\n2\n4\n1\n3\n4\n5\n";
    simplices_out.close();

    // 调用转换函数
    SimplexReader::convertSimplexToHypergraph(nverts_file, simplices_file, output_file);

    // 验证输出文件内容
    ifstream output_in(output_file);
    ASSERT_TRUE(output_in.is_open());

    string line;
    vector<string> lines;
    while (getline(output_in, line))
    {
        lines.push_back(line);
    }
    output_in.close();

    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "1 2 3");
    EXPECT_EQ(lines[1], "2 4");
    EXPECT_EQ(lines[2], "1 3 4 5");

    // 清理测试文件
    remove(nverts_file.c_str());
    remove(simplices_file.c_str());
    remove(output_file.c_str());
}

TEST_F(HypergraphTest, LoadHypergraphFromFile)
{
    GTEST_SKIP() << "Test disabled.";
    // 创建测试文件
    const string test_file = "test_hypergraph.txt";
    ofstream fout(test_file);
    fout << "1 2 3\n2 4\n1 3 4 5\n";
    fout.close();

    // 从文件加载超图

    Hypergraph hg = Hypergraph::fromFile(test_file);

    // 验证超图内容
    EXPECT_EQ(hg.numVertices(), 6); // 顶点编号从1到5，实际顶点数为6（包含0）
    EXPECT_EQ(hg.numHyperedges(), 3);

    auto &edge0 = hg.getHyperedge(0);
    ASSERT_EQ(edge0.vertices.size(), 3);
    EXPECT_EQ(edge0.vertices[0], 1);
    EXPECT_EQ(edge0.vertices[1], 2);
    EXPECT_EQ(edge0.vertices[2], 3);

    auto &edge1 = hg.getHyperedge(1);
    ASSERT_EQ(edge1.vertices.size(), 2);
    EXPECT_EQ(edge1.vertices[0], 2);
    EXPECT_EQ(edge1.vertices[1], 4);

    auto &edge2 = hg.getHyperedge(2);
    ASSERT_EQ(edge2.vertices.size(), 4);
    EXPECT_EQ(edge2.vertices[0], 1);
    EXPECT_EQ(edge2.vertices[1], 3);
    EXPECT_EQ(edge2.vertices[2], 4);
    EXPECT_EQ(edge2.vertices[3], 5);

    // 清理测试文件
    remove(test_file.c_str());
}

// --- 性能对比测试 ---
TEST_F(HypergraphTest, PerformanceComparison)
{
    // GTEST_SKIP() << "Test disabled.";
    // 加载超图数据
    // const string hypergraph_file = "/root/ReachCompress/Edges/Hyper/test1";
    string hypergraph_file = "/root/ReachCompress/Edges/Hyper/random_hypergraph_4";
    Hypergraph hg;
    try
    {
        hg = Hypergraph::fromFile(hypergraph_file);
    }
    catch (const std::exception &e)
    {
        cerr << "Error loading hypergraph: " << e.what() << endl;
        FAIL() << "Failed to load hypergraph file: " << hypergraph_file;
        return;
    }

    cout << "Hypergraph loaded: " << hg.numVertices() << " vertices, "
         << hg.numHyperedges() << " hyperedges." << endl;

    if (hg.numVertices() == 0 || hg.numHyperedges() == 0)
    {
        cout << "Hypergraph is empty, skipping performance test." << endl;
        GTEST_SKIP() << "Skipping performance test on empty hypergraph.";
        return;
    }

    // --- Build Indices Concurrently ---
    cout << "Building indices concurrently (Layered DS, UWeightedPLL, TreeIndex)..." << endl;

    std::chrono::milliseconds build_duration_baseline(0);
    std::chrono::milliseconds build_duration_pll(0);
    std::chrono::milliseconds build_duration_tree(0);
    std::atomic<bool> baseline_build_error(false);
    std::atomic<bool> pll_build_error(false);
    std::atomic<bool> tree_build_error(false);

    // Create tree_index instance before launching thread
    std::unique_ptr<HypergraphTreeIndex> tree_index;
    try
    {
        tree_index = std::make_unique<HypergraphTreeIndex>(hg);
    }
    catch (const std::exception &e)
    {
        cerr << "Error creating HypergraphTreeIndex object: " << e.what() << endl;
        FAIL() << "Failed to create HypergraphTreeIndex object.";
        return;
    }

    // Thread for Layered DS (baseline) build
    std::thread baseline_thread([&hg, &build_duration_baseline, &baseline_build_error, &hypergraph_file]()
                                {
        auto start = high_resolution_clock::now();
        try {
             // Assuming offline_industry was split into public methods
             hg.offline_industry_baseline(hypergraph_file);
        } catch (const std::exception& e) {
             cerr << "Error in baseline build thread: " << e.what() << endl;
             baseline_build_error = true;
        }
        auto end = high_resolution_clock::now();
        build_duration_baseline = duration_cast<milliseconds>(end - start);
        cout << "Layered DS build completed." << endl; });

    // Thread for UWeightedPLL build
    std::thread pll_thread([&hg, &build_duration_pll, &pll_build_error , &hypergraph_file]()
                           {
        auto start = high_resolution_clock::now();
         try {
            // Assuming offline_industry was split into public methods
            hg.offline_industry_pll(hypergraph_file);
         } catch (const std::exception& e) {
             cerr << "Error in PLL build thread: " << e.what() << endl;
             pll_build_error = true;
         }
        auto end = high_resolution_clock::now();
        build_duration_pll = duration_cast<milliseconds>(end - start);
        cout<< "PLL build completed." << endl; });

    // Thread for HypergraphTreeIndex build
    // Pass raw pointer as unique_ptr cannot be copied into lambda capture easily
    HypergraphTreeIndex *tree_index_ptr = tree_index.get();
    std::thread tree_thread([tree_index_ptr, &build_duration_tree, &tree_build_error, &hypergraph_file]()
                            {
         if (!tree_index_ptr) {
             cerr << "Error: tree_index_ptr is null in thread." << endl;
             tree_build_error = true;
             return;
         }
         auto start = high_resolution_clock::now();
         try {
             tree_index_ptr->buildIndexCacheSizeOnly(hypergraph_file);
         } catch (const std::exception& e) {
             cerr << "Error in TreeIndex build thread: " << e.what() << endl;
             tree_build_error = true;
         }
         auto end = high_resolution_clock::now();
         build_duration_tree = duration_cast<milliseconds>(end - start);
         cout << "TreeIndex build completed." << endl; });

    // Wait for all threads to complete
    baseline_thread.join();
    pll_thread.join();
    tree_thread.join();

    // Check for errors during build
    if (baseline_build_error || pll_build_error || tree_build_error)
    {
        FAIL() << "Error occurred during concurrent index building. Check logs.";
        return;
    }

    cout << "Index building finished." << endl;
    cout << " - Layered DS build time:          " << build_duration_baseline.count() << " ms." << endl;
    cout << " - UWeightedPLL build time:        " << build_duration_pll.count() << " ms." << endl;
    cout << " - HypergraphTreeIndex build time: " << build_duration_tree.count() << " ms." << endl;

    // --- 估算 BFS 运行时内存 ---
    size_t N = hg.numVertices();
    size_t bfs_visited_mem = 2 * N * sizeof(bool);
    size_t bfs_pred_dist_mem = 4 * N * sizeof(int);
    size_t bfs_queue_mem_estimate = N * sizeof(int) * 2;
    size_t bfs_meeting_mem_estimate = 100 * sizeof(std::tuple<int, int, int>);
    size_t bfs_total_runtime_mem_bytes = bfs_visited_mem + bfs_pred_dist_mem + bfs_queue_mem_estimate + bfs_meeting_mem_estimate;
    double bfs_total_runtime_mem_mb = static_cast<double>(bfs_total_runtime_mem_bytes) / (1024.0 * 1024.0);

    // --- 打印内存占用 ---
    cout << "\n--- Estimated Memory Usage ---" << endl;
    cout << fixed << setprecision(2);
    cout << "1. BFS (Runtime Estimation):        " << setw(8) << bfs_total_runtime_mem_mb << " MB" << endl;
    cout << "   - Primarily temporary arrays/queues during query execution." << endl;
    cout << "   - Calculation: 2*N*sizeof(bool) + 4*N*sizeof(int) + QueueEst + MeetingPtsEst" << endl;

    cout << "2. Layered DS Index (Total):        " << setw(8) << hg.getWeightedGraphsMemoryUsageMB() << " MB" << endl;
    cout << "   - Consists of " << (Hypergraph::MAX_INTERSECTION_SIZE) << " WeightedGraph layers (k=1 to "
         << Hypergraph::MAX_INTERSECTION_SIZE << ") (Adj Lists + DS)." << endl;
    for (int k = 1; k <= Hypergraph::MAX_INTERSECTION_SIZE; ++k)
    {
        double adj_mem = hg.getWeightedGraphAdjListMemoryUsageMB(k);
        double ds_mem = hg.getWeightedGraphDsMemoryUsageMB(k);
        cout << "   - Layer " << setw(2) << k << ": AdjList=" << setw(7) << adj_mem << " MB, DS=" << setw(7) << ds_mem << " MB" << endl;
    }
    cout << "   - AdjList Calc: sizeof(vec) + N*(sizeof(inner_vec) + cap*sizeof(pair)) per layer" << endl;
    cout << "   - DS Calc: sizeof(DS_obj) + parent_cap*sizeof(int) + rank_cap*sizeof(int) per layer" << endl;

    cout << "3. UWeightedPLL Index:            " << setw(8) << hg.getPllMemoryUsageMB() << " MB" << endl;
    cout << "   - Consists of the PLL graph (AdjList only) + PLL labels." << endl;
    cout << "   - Total Labels (pairs):        " << hg.getPllTotalLabelSize() << endl;
    cout << "   - Calc: PLLGraphAdjListMem + sizeof(label_vec) + N*(sizeof(inner_vec) + cap*sizeof(pair))" << endl;

    double tree_index_mem_mb = 0.0;
    size_t tree_index_nodes = 0;
    if (tree_index)
    {
        tree_index_mem_mb = tree_index->getMemoryUsageMB();
        tree_index_nodes = tree_index->getTotalNodes();
    }
    cout << "4. HypergraphTreeIndex:           " << setw(8) << tree_index_mem_mb << " MB" << endl;
    cout << "   - Consists of tree nodes, LCA structures (up array), DSU, etc." << endl;
    cout << "   - Total Tree Nodes:            " << tree_index_nodes << endl;
    cout << "   - Calc: Sum of nodes, pointers, vectors (using capacity)" << endl;

    // // --- 准备查询 ---
    // const int num_queries = 3000;
    // std::vector<std::tuple<int, int, int>> queries;
    // std::mt19937 rng(std::random_device{}());
    // std::uniform_int_distribution<int> vertex_dist(0, hg.numVertices() - 1);
    // std::uniform_int_distribution<int> k_dist(3, Hypergraph::MAX_INTERSECTION_SIZE);

    // cout << "\nGenerating " << num_queries << " random queries (k range [1, " << Hypergraph::MAX_INTERSECTION_SIZE << "])..." << endl;
    // for (int i = 0; i < num_queries; ++i) {
    //     int u = vertex_dist(rng);
    //     int v = vertex_dist(rng);
    //     int k = k_dist(rng);
    //     queries.emplace_back(u, v, k);
    // }


// ...existing code...
    // --- Generate Specific Queries ---
    const int num_queries_per_group = 1000;
    const int target_k_values[] = {2, 4, 6, 8, 10};
    const std::string query_types[] = {"Random", "Unreachable", "Reachable"};  // 将 "Exact" 改为 "Random"
    const int num_k_values = sizeof(target_k_values) / sizeof(target_k_values[0]);
    const int num_query_types = sizeof(query_types) / sizeof(query_types[0]);
    const int total_queries_target = num_query_types * num_k_values * num_queries_per_group;
    // 设置连续失败次数阈值，达到此阈值则放弃当前 k 值的生成
    const int consecutive_failure_threshold = 2000;
    // Store queries with their intended group: (u, v, k_for_query, type_string, k_group)
    std::vector<std::tuple<int, int, int, std::string, int>> queries;
    queries.reserve(total_queries_target);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> vertex_dist(0, hg.numVertices() - 1);

    // Keep track of generated counts for each group
    std::map<std::pair<int, std::string>, int> generated_counts;
    for (int k : target_k_values)
    {
        for (const auto &type : query_types)
        {
            generated_counts[{k, type}] = 0;
        }
    }

    cout << "\nGenerating up to " << total_queries_target << " specific queries ("
         << num_queries_per_group << " per k/type group)..." << endl;

    for (int target_k : target_k_values)
    {
        cout << "  Generating queries for k=" << target_k << "..." << endl;
        int consecutive_generation_failures = 0; // 为每个 k 重置连续失败计数器

        // 只要当前 k 值下还有任何类型的查询未生成满，就继续尝试
        while (generated_counts[{target_k, "Random"}] < num_queries_per_group ||
               generated_counts[{target_k, "Unreachable"}] < num_queries_per_group ||
               generated_counts[{target_k, "Reachable"}] < num_queries_per_group)
        {
            // 检查是否达到连续失败阈值
            if (consecutive_generation_failures >= consecutive_failure_threshold)
            {
                cout << "    Warning: Exceeded " << consecutive_failure_threshold
                     << " consecutive generation failures for k=" << target_k
                     << ". Moving to next k value." << endl;
                break; // 跳出当前 k 值的 while 循环
            }

            int u = vertex_dist(rng);
            int v = vertex_dist(rng);
            if (u == v)
            {
                // 如果生成了相同的顶点，也算作一次失败尝试（因为我们需要不同的顶点对）
                consecutive_generation_failures++;
                continue;
            }

            bool reachable_at_k = false;
            bool query_error = false;

            try
            {
                reachable_at_k = hg.isReachableViaUWeightedPLL(u, v, target_k);
            }
            catch (const std::exception &e)
            {
                query_error = true;
                // PLL 查询出错，跳过此对，但不计入连续失败次数
            }

            if (query_error)
                continue;

            bool pair_added_this_attempt = false;

            // 1. Check Random (不管可达性，只要是随机点对)
            if (generated_counts[{target_k, "Random"}] < num_queries_per_group)
            {
                queries.emplace_back(u, v, target_k, "Random", target_k);
                generated_counts[{target_k, "Random"}]++;
                pair_added_this_attempt = true;
                // 注意：即使是随机添加的，我们仍然要检查可达性情况，所以不在这里continue
            }

            // 2. Check Unreachable k (只有尚未满足数量要求时才添加)
            if (!reachable_at_k && generated_counts[{target_k, "Unreachable"}] < num_queries_per_group)
            {
                queries.emplace_back(u, v, target_k, "Unreachable", target_k);
                generated_counts[{target_k, "Unreachable"}]++;
                pair_added_this_attempt = true;
            }

            // 3. Check Reachable k (只有尚未满足数量要求时才添加)
            if (reachable_at_k && generated_counts[{target_k, "Reachable"}] < num_queries_per_group)
            {
                queries.emplace_back(u, v, target_k, "Reachable", target_k);
                generated_counts[{target_k, "Reachable"}]++;
                pair_added_this_attempt = true;
            }

            // 更新连续失败计数器
            if (pair_added_this_attempt)
            {
                consecutive_generation_failures = 0; // 成功添加，重置计数器
            }
            else
            {
                // 只有在当前 k 值下仍有未满的查询类型时，才增加失败计数
                if (generated_counts[{target_k, "Random"}] < num_queries_per_group ||
                    generated_counts[{target_k, "Unreachable"}] < num_queries_per_group ||
                    generated_counts[{target_k, "Reachable"}] < num_queries_per_group)
                {
                    consecutive_generation_failures++;
                }
            }
        } // End while loop for current k

        // 报告当前 k 值的最终生成数量
        cout << "    - Random k:       Generated " << generated_counts[{target_k, "Random"}] << " / " << num_queries_per_group << endl;
        cout << "    - Unreachable k: Generated " << generated_counts[{target_k, "Unreachable"}] << " / " << num_queries_per_group << endl;
        cout << "    - Reachable k:   Generated " << generated_counts[{target_k, "Reachable"}] << " / " << num_queries_per_group << endl;
        
        // 如果是因为连续失败而退出，可以加一个提示
        if (consecutive_generation_failures >= consecutive_failure_threshold)
        {
            cout << "    -> Stopped generating for k=" << target_k << " due to consecutive failures." << endl;
        }
    } // End loop for target_k
// ...existing code...


    cout << "Generated a total of " << queries.size() << " queries." << endl;

    // --- 执行并计时查询 ---
    cout << "\nRunning " << queries.size() << " queries and grouping results..." << endl;
    // Data structure to store timings and counts per method/k/type
    // method_name -> {k_group, type_string} -> {total_ns, count}
    std::map<std::string, std::map<std::pair<int, std::string>, std::pair<long long, int>>> timings;
    // Data structure to store true counts per method/k/type
    std::map<std::string, std::map<std::pair<int, std::string>, int>> true_counts;

    std::vector<std::string> method_names = {"BFS", "LayeredDS", "UWeightedPLL", "TreeIndex"};

    // Initialize maps
    for (const auto &method : method_names)
    {
        for (int k : target_k_values)
        {
            for (const auto &type : query_types)
            {
                timings[method][{k, type}] = {0, 0};
                true_counts[method][{k, type}] = 0;
            }
        }
    }

    // --- Execute BFS ---
    cout << "  Executing BFS..." << endl;
    for (const auto &q_tuple : queries)
    {
        int u, v, k_query, k_group;
        std::string type;
        std::tie(u, v, k_query, type, k_group) = q_tuple;
        bool result = false;
        auto start_ns = high_resolution_clock::now();
        try
        {
            result = hg.isReachableBidirectionalBFS(u, v, k_query);
        }
        catch (...)
        { /* Ignore errors during timing */
        }
        auto end_ns = high_resolution_clock::now();
        long long delta_ns = duration_cast<nanoseconds>(end_ns - start_ns).count();
        timings["BFS"][{k_group, type}].first += delta_ns;
        timings["BFS"][{k_group, type}].second++;
        if (result)
            true_counts["BFS"][{k_group, type}]++;
    }

    // --- Execute Layered DS ---
    cout << "  Executing LayeredDS..." << endl;
    for (const auto &q_tuple : queries)
    {
        int u, v, k_query, k_group;
        std::string type;
        std::tie(u, v, k_query, type, k_group) = q_tuple;
        bool result = false;
        auto start_ns = high_resolution_clock::now();
        try
        {
            result = hg.isReachableViaWeightedGraph(u, v, k_query);
        }
        catch (...)
        { /* Ignore errors */
        }
        auto end_ns = high_resolution_clock::now();
        long long delta_ns = duration_cast<nanoseconds>(end_ns - start_ns).count();
        timings["LayeredDS"][{k_group, type}].first += delta_ns;
        timings["LayeredDS"][{k_group, type}].second++;
        if (result)
            true_counts["LayeredDS"][{k_group, type}]++;
    }

    // --- Execute UWeightedPLL ---
    cout << "  Executing UWeightedPLL..." << endl;
    for (const auto &q_tuple : queries)
    {
        int u, v, k_query, k_group;
        std::string type;
        std::tie(u, v, k_query, type, k_group) = q_tuple;
        bool result = false;
        auto start_ns = high_resolution_clock::now();
        try
        {
            result = hg.isReachableViaUWeightedPLL(u, v, k_query);
        }
        catch (...)
        { /* Ignore errors */
        }
        auto end_ns = high_resolution_clock::now();
        long long delta_ns = duration_cast<nanoseconds>(end_ns - start_ns).count();
        timings["UWeightedPLL"][{k_group, type}].first += delta_ns;
        timings["UWeightedPLL"][{k_group, type}].second++;
        if (result)
            true_counts["UWeightedPLL"][{k_group, type}]++;
    }

    // --- Execute Tree Index ---
    cout << "  Executing TreeIndex..." << endl;
    for (const auto &q_tuple : queries)
    {
        int u, v, k_query, k_group;
        std::string type;
        std::tie(u, v, k_query, type, k_group) = q_tuple;
        bool result = false;
        auto start_ns = high_resolution_clock::now();
        try
        {
            result = tree_index->querySizeOnly(u, v, k_query);
        }
        catch (...)
        { /* Ignore errors */
        }
        auto end_ns = high_resolution_clock::now();
        long long delta_ns = duration_cast<nanoseconds>(end_ns - start_ns).count();
        timings["TreeIndex"][{k_group, type}].first += delta_ns;
        timings["TreeIndex"][{k_group, type}].second++;
        if (result)
            true_counts["TreeIndex"][{k_group, type}]++;
    }

    // --- Print Detailed Query Performance Results ---
    cout << "\n--- Detailed Query Performance ---" << endl;
    cout << fixed << setprecision(3);

    for (int k : target_k_values)
    {
        cout << "\n--- k = " << k << " ---" << endl;
        cout << "  Type         | Method          | Avg. Time (µs) | Count | True Results" << endl;
        cout << "-----------------|-----------------|----------------|-------|--------------" << endl;
        for (const auto &type : query_types)
        {
            for (const auto &method : method_names)
            {
                long long total_ns = timings[method][{k, type}].first;
                int count = timings[method][{k, type}].second;
                int trues = true_counts[method][{k, type}];
                double avg_us = (count == 0) ? 0.0 : (double)total_ns / count / 1000.0;

                cout << "  " << left << setw(14) << type
                     << "| " << left << setw(15) << method
                     << "| " << right << setw(14) << avg_us
                     << " | " << right << setw(5) << count
                     << " | " << right << setw(12) << trues << endl;
            }
            if (&type != &query_types[num_query_types - 1])
            { // Add separator between types within the same k
                cout << "-----------------|-----------------|----------------|-------|--------------" << endl;
            }
        }
    }

    // --- Verify Result Consistency (Keep as is, checks first few overall queries) ---
    cout << "\nVerifying result consistency (first 10 overall queries)..." << endl;
    bool consistent = true;
    for (int i = 0; i < std::min((int)queries.size(), 10); ++i)
    {
        int u, v, k_query, k_group;
        std::string type;
        std::tie(u, v, k_query, type, k_group) = queries[i];
        bool bfs_res = false, layered_ds_res = false, uweighted_pll_res = false, tree_index_res = false;
        bool bfs_ok = true, layered_ds_ok = true, pll_ok = true, tree_ok = true;

        try
        {
            bfs_res = hg.isReachableBidirectionalBFS(u, v, k_query);
        }
        catch (...)
        {
            bfs_ok = false;
        }
        try
        {
            layered_ds_res = hg.isReachableViaWeightedGraph(u, v, k_query);
        }
        catch (...)
        {
            layered_ds_ok = false;
        }
        try
        {
            uweighted_pll_res = hg.isReachableViaUWeightedPLL(u, v, k_query);
        }
        catch (...)
        {
            pll_ok = false;
        }
        try
        {
            tree_index_res = tree_index->querySizeOnly(u, v, k_query);
        }
        catch (...)
        {
            tree_ok = false;
        }

        if (bfs_ok && layered_ds_ok && pll_ok && tree_ok)
        {
            if (!(bfs_res == layered_ds_res && bfs_res == uweighted_pll_res && bfs_res == tree_index_res))
            {
                cout << "  Inconsistency found for query (" << u << ", " << v << ", k=" << k_query << ", type=" << type << ", k_group=" << k_group << "): "
                     << "BFS=" << bfs_res << ", LayeredDS=" << layered_ds_res << ", UWeightedPLL=" << uweighted_pll_res
                     << ", TreeIndex=" << tree_index_res << endl;
                consistent = false;
            }
        }
        else
        {
            cout << "  Skipping consistency check for query (" << u << ", " << v << ", k=" << k_query << ") due to query execution error." << endl;
        }
    }
    if (consistent)
    {
        cout << "  Results appear consistent for the first 10 queries (where all methods succeeded)." << endl;
    }
    else
    {
        ADD_FAILURE() << "Query results are inconsistent between methods!";
    }

    cout << "\n--- Performance Comparison Test End ---" << endl;
}

// New Test Fixture for HypergraphTreeIndex
class HypergraphTreeIndexTest : public ::testing::Test
{
protected:
    Hypergraph hg;
    std::unique_ptr<HypergraphTreeIndex> index;

    void SetUp() override
    {
        hg = Hypergraph(10, 10);
        hg.addVertices(6);

        hg.addHyperedge({0, 1, 2});
        hg.addHyperedge({1, 2, 3});
        hg.addHyperedge({3, 4});
        hg.addHyperedge({4, 5});
        hg.addHyperedge({0, 5});

        index = std::make_unique<HypergraphTreeIndex>(hg);
        index->buildIndexCacheSizeOnly(PROJECT_ROOT_DIR"/IndexCache/test_hypergraph_tree_index");
    }

    void TearDown() override
    {
    }
};

TEST_F(HypergraphTreeIndexTest, BasicQueries)
{
    GTEST_SKIP() << "Test disabled.";
    ASSERT_NE(index, nullptr);

    EXPECT_TRUE(index->query(0, 3, 1));
    EXPECT_TRUE(index->query(0, 3, 2));
    EXPECT_FALSE(index->query(0, 3, 3));
    EXPECT_TRUE(index->query(1, 3, 1));
    EXPECT_TRUE(index->query(1, 3, 2));

    EXPECT_TRUE(index->query(1, 4, 1));
    EXPECT_FALSE(index->query(1, 4, 2));
    EXPECT_TRUE(index->query(2, 4, 1));

    EXPECT_TRUE(index->query(3, 5, 1));
    EXPECT_FALSE(index->query(3, 5, 2));

    EXPECT_TRUE(index->query(0, 5, 1));
    EXPECT_TRUE(index->query(0, 5, 2));

    EXPECT_TRUE(index->query(0, 1, 1));
    EXPECT_TRUE(index->query(0, 1, 10));
    EXPECT_TRUE(index->query(4, 5, 1));
    EXPECT_TRUE(index->query(4, 5, 5));

    EXPECT_TRUE(index->query(0, 5, 1));

    EXPECT_TRUE(index->query(1, 5, 1));
    EXPECT_FALSE(index->query(1, 5, 2));

    EXPECT_TRUE(index->query(0, 0, 1));
    EXPECT_TRUE(index->query(3, 3, 5));
}



int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
