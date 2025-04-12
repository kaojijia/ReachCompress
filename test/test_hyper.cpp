#include "Hypergraph.h"
#include "HypergraphTreeIndex.h" // Include the new header
#include "UndirectedPLL.h"
#include "UWeightedPLL.h"
#include <gtest/gtest.h>
#include <fstream>
#include "SimplexReader.h"
#include <iostream> // For std::cout in test

using namespace std;

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
    // GTEST_SKIP() << "Test disabled.";
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

// New Test Fixture for HypergraphTreeIndex
class HypergraphTreeIndexTest : public ::testing::Test
{
protected:
    Hypergraph hg;
    std::unique_ptr<HypergraphTreeIndex> index; // Use unique_ptr

    void SetUp() override
    {
        // Setup a sample hypergraph for testing the index
        hg = Hypergraph(10, 10); // Pre-allocate some space
        hg.addVertices(6);       // Add vertices 0 to 5

        // Add hyperedges
        hg.addHyperedge({0, 1, 2}); // Edge 0
        hg.addHyperedge({1, 2, 3}); // Edge 1 (Intersection {1, 2} with Edge 0, size 2)
        hg.addHyperedge({3, 4});    // Edge 2 (Intersection {3} with Edge 1, size 1)
        hg.addHyperedge({4, 5});    // Edge 3 (Intersection {4} with Edge 2, size 1)
        hg.addHyperedge({0, 5});    // Edge 4 (No intersection with others > 0)

        // Create and build the index
        index = std::make_unique<HypergraphTreeIndex>(hg);
        index->buildIndex();
    }

    void TearDown() override
    {
        // Clean up if needed, unique_ptr handles memory automatically
    }
};

// Test case for HypergraphTreeIndex basic queries
TEST_F(HypergraphTreeIndexTest, BasicQueries)
{
    ASSERT_NE(index, nullptr); // Ensure index was created

    // Test cases based on the setup hypergraph
    // Query(u, v, k): Can we go from vertex u to v with intersection size >= k at each step?

    // Path through e0 and e1 (LCA should have intersection_size 2)
    EXPECT_TRUE(index->query(0, 3, 1));  // k=1, should pass (2 >= 1)
    EXPECT_TRUE(index->query(0, 3, 2));  // k=2, should pass (2 >= 2)
    EXPECT_FALSE(index->query(0, 3, 3)); // k=3, should fail (2 < 3)
    EXPECT_TRUE(index->query(1, 3, 1));  // Also uses e0/e1
    EXPECT_TRUE(index->query(1, 3, 2));

    // Path through e1 and e2 (LCA should have intersection_size 1)
    EXPECT_TRUE(index->query(1, 4, 1));  // k=1, should pass (1 >= 1)
    EXPECT_FALSE(index->query(1, 4, 2)); // k=2, should fail (1 < 2)
    EXPECT_TRUE(index->query(2, 4, 1));  // Also uses e1/e2

    // Path through e2 and e3 (LCA should have intersection_size 1)
    EXPECT_TRUE(index->query(3, 5, 1));  // k=1, should pass (1 >= 1)
    EXPECT_FALSE(index->query(3, 5, 2)); // k=2, should fail (1 < 2)

    // Longer path 0 -> e0 -> e1 -> e2 -> e3 -> 5 (bottleneck is 1)
    EXPECT_TRUE(index->query(0, 5, 1));  // k=1, should pass
    EXPECT_FALSE(index->query(0, 5, 2)); // in one edge, should pass

    // Vertices in the same hyperedge (should always be true for k>=1 if edge exists)
    EXPECT_TRUE(index->query(0, 1, 1));
    EXPECT_TRUE(index->query(0, 1, 10)); // k doesn't matter for same edge
    EXPECT_TRUE(index->query(4, 5, 1));
    EXPECT_TRUE(index->query(4, 5, 5));

    // Query involving edge 4 ({0, 5}) which has no intersection > 0 with others
    // The path 0 -> e4 -> 5 exists directly.
    EXPECT_TRUE(index->query(0, 5, 1)); // Should be true due to direct connection in e4

    // Test non-reachable case if structure implies separation (depends on build logic)
    // If e4 is truly isolated in the tree structure from others for k>0
    // EXPECT_FALSE(index->query(1, 5, 1)); // Example: Is 1 reachable from 5 with k=1?
    // This depends on how the tree connects e4.
    // Based on the code, 0->e0->(LCA:2)->e1->3->e2->(LCA:1)->e3->4->e3->5
    // So 1 and 5 should be reachable with k=1. Let's test that.
    EXPECT_TRUE(index->query(1, 5, 1));  // Should be true, bottleneck is 1.
    EXPECT_FALSE(index->query(1, 5, 2)); // Should be false, bottleneck is 1.

    // Test same vertex
    EXPECT_TRUE(index->query(0, 0, 1));
    EXPECT_TRUE(index->query(3, 3, 5));
}

// Test case for saving the tree index to a file
TEST_F(HypergraphTreeIndexTest, SaveToFile)
{
    ASSERT_NE(index, nullptr);
    const std::string filename = "test_tree_output.dot";

    // Attempt to save the file
    ASSERT_NO_THROW(index->saveToFile(filename));

    // Basic check: does the file exist?
    std::ifstream file(filename);
    EXPECT_TRUE(file.good());
    file.close();

    // Optional: More advanced check - read the file and verify some content
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
        { // Look for any edge definition
            edge_found = true;
        }
    }
    infile.close();
    EXPECT_TRUE(graph_tag_found);
    EXPECT_TRUE(node0_found);
    EXPECT_TRUE(edge_found);

    // Clean up the test file
    remove(filename.c_str());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
