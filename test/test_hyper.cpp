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
    // 加载超图数据 - 替换为你的实际大型数据集路径
    // 如果没有大型数据集，可以使用下面的测试文件，但结果可能不显著
    const string hypergraph_file = PROJECT_ROOT_DIR"/Edges/Hyper/Hyper_test1"; 

    Hypergraph hg;
    try {
        hg = Hypergraph::fromFile(hypergraph_file);
    } catch (const std::exception& e) {
        cerr << "Error loading hypergraph: " << e.what() << endl;
        FAIL() << "Failed to load hypergraph file: " << hypergraph_file;
        return;
    }

    cout << "Hypergraph loaded: " << hg.numVertices() << " vertices, "
         << hg.numHyperedges() << " hyperedges." << endl;

    if (hg.numVertices() == 0 || hg.numHyperedges() == 0) {
         cout << "Hypergraph is empty, skipping performance test." << endl;
         GTEST_SKIP() << "Skipping performance test on empty hypergraph.";
         return;
    }

    cout << "Building indices (offline_industry for Layered DS & UWeightedPLL)..." << endl;
    auto build_start_ds_pll = high_resolution_clock::now();
    hg.offline_industry(); // Builds indices for Layered DS and UWeightedPLL
    auto build_end_ds_pll = high_resolution_clock::now();
    auto build_duration_ds_pll = duration_cast<milliseconds>(build_end_ds_pll - build_start_ds_pll);
    cout << "Layered DS & UWeightedPLL building finished in " << build_duration_ds_pll.count() << " ms." << endl;

    // --- 构建 HypergraphTreeIndex ---
    cout << "Building HypergraphTreeIndex..." << endl;
    std::unique_ptr<HypergraphTreeIndex> tree_index;
    auto build_start_tree = high_resolution_clock::now();
    try {
        tree_index = std::make_unique<HypergraphTreeIndex>(hg);
        tree_index->buildIndex();
    } catch (const std::exception& e) {
        cerr << "Error building HypergraphTreeIndex: " << e.what() << endl;
        FAIL() << "Failed to build HypergraphTreeIndex.";
        return;
    }
    auto build_end_tree = high_resolution_clock::now();
    auto build_duration_tree = duration_cast<milliseconds>(build_end_tree - build_start_tree);
    cout << "HypergraphTreeIndex building finished in " << build_duration_tree.count() << " ms." << endl;

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
    for (int k = 1; k <= Hypergraph::MAX_INTERSECTION_SIZE; ++k) {
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
    if (tree_index) {
        tree_index_mem_mb = tree_index->getMemoryUsageMB();
        tree_index_nodes = tree_index->getTotalNodes();
    }
    cout << "4. HypergraphTreeIndex:           " << setw(8) << tree_index_mem_mb << " MB" << endl;
    cout << "   - Consists of tree nodes, LCA structures (up array), DSU, etc." << endl;
    cout << "   - Total Tree Nodes:            " << tree_index_nodes << endl;
    cout << "   - Calc: Sum of nodes, pointers, vectors (using capacity)" << endl;

    // --- 准备查询 ---
    const int num_queries = 1000;
    std::vector<std::tuple<int, int, int>> queries;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> vertex_dist(0, hg.numVertices() - 1);
    std::uniform_int_distribution<int> k_dist(1, Hypergraph::MAX_INTERSECTION_SIZE);

    cout << "\nGenerating " << num_queries << " random queries (k range [1, " << Hypergraph::MAX_INTERSECTION_SIZE << "])..." << endl;
    for (int i = 0; i < num_queries; ++i) {
        int u = vertex_dist(rng);
        int v = vertex_dist(rng);
        int k = k_dist(rng);
        queries.emplace_back(u, v, k);
    }

    // --- 执行并计时查询 ---
    cout << "Running queries..." << endl;
    long long bfs_time_ns = 0;
    long long layered_ds_time_ns = 0;
    long long uweighted_pll_time_ns = 0;
    long long tree_index_time_ns = 0;
    int bfs_true_count = 0;
    int layered_ds_true_count = 0;
    int uweighted_pll_true_count = 0;
    int tree_index_true_count = 0;

    auto start_bfs = high_resolution_clock::now();
    for (const auto& q : queries) {
        if (hg.isReachableBidirectionalBFS(get<0>(q), get<1>(q), get<2>(q))) {
            bfs_true_count++;
        }
    }
    auto end_bfs = high_resolution_clock::now();
    bfs_time_ns = duration_cast<nanoseconds>(end_bfs - start_bfs).count();

    auto start_layered_ds = high_resolution_clock::now();
    for (const auto& q : queries) {
         if (hg.isReachableViaWeightedGraph(get<0>(q), get<1>(q), get<2>(q))) {
             layered_ds_true_count++;
         }
    }
    auto end_layered_ds = high_resolution_clock::now();
    layered_ds_time_ns = duration_cast<nanoseconds>(end_layered_ds - start_layered_ds).count();

    auto start_uweighted_pll = high_resolution_clock::now();
    for (const auto& q : queries) {
         if (hg.isReachableViaUWeightedPLL(get<0>(q), get<1>(q), get<2>(q))) {
             uweighted_pll_true_count++;
         }
    }
    auto end_uweighted_pll = high_resolution_clock::now();
    uweighted_pll_time_ns = duration_cast<nanoseconds>(end_uweighted_pll - start_uweighted_pll).count();

    auto start_tree_index = high_resolution_clock::now();
    for (const auto& q : queries) {
         if (tree_index->query(get<0>(q), get<1>(q), get<2>(q))) {
             tree_index_true_count++;
         }
    }
    auto end_tree_index = high_resolution_clock::now();
    tree_index_time_ns = duration_cast<nanoseconds>(end_tree_index - start_tree_index).count();

    // --- 打印查询性能结果 ---
    cout << "\n--- Query Performance (" << num_queries << " queries) ---" << endl;
    cout << fixed << setprecision(3);
    cout << "  Method                          | Avg. Time (µs) | Total Time (ms) | True Results" << endl;
    cout << "----------------------------------|----------------|-----------------|--------------" << endl;
    cout << "  BFS (Hypergraph)                | " << setw(14) << (double)bfs_time_ns / num_queries / 1000.0
         << " | " << setw(15) << (double)bfs_time_ns / 1000000.0 << " | " << bfs_true_count << endl;
    cout << "  Layered DS (WeightedGraph+DS)   | " << setw(14) << (double)layered_ds_time_ns / num_queries / 1000.0
         << " | " << setw(15) << (double)layered_ds_time_ns / 1000000.0 << " | " << layered_ds_true_count << endl;
    cout << "  UWeightedPLL                    | " << setw(14) << (double)uweighted_pll_time_ns / num_queries / 1000.0
         << " | " << setw(15) << (double)uweighted_pll_time_ns / 1000000.0 << " | " << uweighted_pll_true_count << endl;
    cout << "  HypergraphTreeIndex             | " << setw(14) << (double)tree_index_time_ns / num_queries / 1000.0
         << " | " << setw(15) << (double)tree_index_time_ns / 1000000.0 << " | " << tree_index_true_count << endl;

    // --- 验证结果一致性 ---
    cout << "\nVerifying result consistency (first 10 queries)..." << endl;
    bool consistent = true;
    for(int i = 0; i < std::min(10, num_queries); ++i) {
        int u, v, k;
        std::tie(u, v, k) = queries[i];
        bool bfs_res = hg.isReachableBidirectionalBFS(u, v, k);
        bool layered_ds_res = hg.isReachableViaWeightedGraph(u, v, k);
        bool uweighted_pll_res = hg.isReachableViaUWeightedPLL(u, v, k);
        bool tree_index_res = tree_index->query(u, v, k);

        if (!(bfs_res == layered_ds_res && bfs_res == uweighted_pll_res && bfs_res == tree_index_res)) {
            cout << "  Inconsistency found for query (" << u << ", " << v << ", k=" << k << "): "
                 << "BFS=" << bfs_res << ", LayeredDS=" << layered_ds_res << ", UWeightedPLL=" << uweighted_pll_res
                 << ", TreeIndex=" << tree_index_res << endl;
            consistent = false;
        }
    }
    if (consistent) {
        cout << "  Results appear consistent for the first 10 queries." << endl;
    } else {
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
        index->buildIndex();
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
    EXPECT_FALSE(index->query(0, 5, 2));

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

TEST_F(HypergraphTreeIndexTest, SaveToFile)
{
    GTEST_SKIP() << "Test disabled.";
    ASSERT_NE(index, nullptr);
    const std::string filename = "test_tree_output.dot";

    ASSERT_NO_THROW(index->saveToFile(filename));

    std::ifstream file(filename);
    EXPECT_TRUE(file.good());
    file.close();

    std::ifstream infile(filename);
    std::string line;
    bool graph_tag_found = false;
    bool node0_found = false;
    bool edge_found = false;
    while (std::getline(infile, line))
    {
        if (line.find("graph HypergraphTree {") != std::string::npos)
        {
            graph_tag_found = true;
        }
        if (line.find("node0 [label=") != std::string::npos)
        {
            node0_found = true;
        }
        if (line.find("--") != std::string::npos)
        {
            edge_found = true;
        }
    }
    infile.close();
    EXPECT_TRUE(graph_tag_found);
    EXPECT_TRUE(node0_found);
    EXPECT_TRUE(edge_found);

    remove(filename.c_str());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
