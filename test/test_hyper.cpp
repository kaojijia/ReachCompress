#include "Hypergraph.h"
#include "UndirectedPLL.h"
#include <gtest/gtest.h>
#include <fstream>

using namespace std;

class HypergraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        // hg.addVertices(3);
        // edge1 = {0, 1};
        // edge2 = {1, 2};
    }

    // Hypergraph hg;
    // vector<int> edge1;
    // vector<int> edge2;
};

TEST_F(HypergraphTest, BasicOperations) {
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

    auto& edges0 = hg.getIncidentHyperedges(0);
    ASSERT_EQ(edges0.size(), 1);
    EXPECT_EQ(edges0[0], 0);
}

TEST_F(HypergraphTest, ConnectedComponents) {
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg;
    hg.addVertices(6);
    hg.addHyperedge({0,1,2});
    hg.addHyperedge({3,4,5});

    auto components = hg.getConnectedComponents();
    ASSERT_EQ(components.size(), 2);
    EXPECT_EQ(components[0].size(), 3);
    EXPECT_EQ(components[1].size(), 3);
}

TEST_F(HypergraphTest, Reachability) {
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg(0,0);
    // 添加15个顶点
    auto num = hg.addVertices(15);
    EXPECT_EQ(num, 15);
    
    // 连通边的链 {0,1,2}-{2,3}-{4,5,6}
    hg.addHyperedge({0,1,2,3});
    hg.addHyperedge({2,3,4});
    hg.addHyperedge({4,5,6});
    hg.addHyperedge({14,6});
    
    // 环状连通 {7,8,9}-{9,10}-{10,11,7}
    hg.addHyperedge({7,8,9,12});
    hg.addHyperedge({9,10,12}); 
    hg.addHyperedge({10,11,7,12});
    
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
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0,6,2));
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0,6,3));
    EXPECT_FALSE(hg.isReachableBidirectionalBFS(0,14,2));
    EXPECT_TRUE(hg.isReachableBidirectionalBFS(0,6,1));

    EXPECT_FALSE(hg.isReachableBidirectionalBFS(8,11,3));
    EXPECT_TRUE(hg.isReachableBidirectionalBFS(8,11,2));

    //验证算法正确性
    cout << endl;
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            for (int k = 1; k <= 4; ++k) {
                bool bfsResult = hg.isReachableBidirectionalBFS(i, j, k);
                bool graphResult = hg.isReachableViaWeightedGraph(i, j, k);

                EXPECT_EQ(bfsResult, graphResult);
            }
        }
    }


}

TEST_F(HypergraphTest, PLI){
        // 1. 构建一个无向带权图 wg
        // 设定一个边权阈值，比如 5，忽略 weight<5 的边
        WeightedGraph wg(29 /*顶点数*/, 5/*最小可通过的边权重*/);
        wg.addEdge(0, 1, 1);
        wg.addEdge(1, 2, 3);
        wg.addEdge(2, 3, 10);
        wg.addEdge(3, 4, 2);
        wg.addEdge(2, 5, 7);
        //链
        wg.addEdge(5, 6, 19);
        wg.addEdge(6, 7, 19);
        wg.addEdge(7, 8, 19);
        wg.addEdge(8, 9, 19);
        wg.addEdge(9, 10, 19);
        wg.addEdge(10, 20, 19);
        wg.addEdge(20, 21, 19);
        //环
        wg.addEdge(11, 12, 19);
        wg.addEdge(12, 13, 19);
        wg.addEdge(13, 14, 19);
        wg.addEdge(14, 15, 19);
        wg.addEdge(15, 11, 19);
    
        // 2. 创建索引

        wg.offline_industry();
    
        // 3. 查询可达性
        // 链太长了找不到
        EXPECT_TRUE(wg.landmark_reachability_query(2,10));
        EXPECT_TRUE(wg.landmark_reachability_query(5,10));
        EXPECT_TRUE(wg.landmark_reachability_query(10,6));
        EXPECT_TRUE(wg.landmark_reachability_query(5,21));


        EXPECT_TRUE(wg.landmark_reachability_query(11,14));
        EXPECT_FALSE(wg.landmark_reachability_query(0,1));
        EXPECT_TRUE(wg.landmark_reachability_query(2,3));
        EXPECT_TRUE(wg.landmark_reachability_query(3,5));
        EXPECT_FALSE(wg.landmark_reachability_query(0,5));
        EXPECT_FALSE(wg.landmark_reachability_query(0,2));
        EXPECT_FALSE(wg.landmark_reachability_query(0,4));
        EXPECT_FALSE(wg.landmark_reachability_query(1,4));

}

TEST_F(HypergraphTest, FileLoading) {
    GTEST_SKIP() << "Test disabled.";
    const string test_file = "test_hyper.txt";
    ofstream fout(test_file);
    fout << "0 1 2\n3 4\n5 6 7 8";
    fout.close();

    Hypergraph hg = Hypergraph::fromFile(test_file);
    EXPECT_EQ(hg.numVertices(), 9);
    EXPECT_EQ(hg.numHyperedges(), 3);
    
    auto& first_edge = hg.getHyperedge(0);
    ASSERT_EQ(first_edge.vertices.size(), 3);
    EXPECT_EQ(first_edge.vertices[0], 0);
}

TEST_F(HypergraphTest, ExceptionHandling) {
    GTEST_SKIP() << "Test disabled.";
    Hypergraph hg(3);
    
    EXPECT_THROW(hg.addHyperedge({0, 5}), invalid_argument);
    EXPECT_THROW(hg.getHyperedge(10), invalid_argument);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
