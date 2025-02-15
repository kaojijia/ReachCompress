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


TEST(TEST_Set, index_test){
    Graph g;
    vector<pair<int, int>> edges = { {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}};
    edges.push_back(make_pair(4, 11));
    edges.push_back(make_pair(4,12));
    edges.push_back(make_pair(13,4));
    edges.push_back(make_pair(1,13));
    edges.push_back(make_pair(7,15));
    edges.push_back(make_pair(7,16));
    edges.push_back(make_pair(17,7));

    for(auto edge : edges){
        g.addEdge(edge.first, edge.second);
    }
    SetSearch set_search(g);
    set_search.offline_industry();

    EXPECT_TRUE(set_search.reachability_query(1,15));

    vector<int> source_set = {1, 3, 5, 121};
    vector<int> target_set = {2, 8,123,1, 122};

    // auto c = set_search.build_source_forest(source_set);
    auto result = set_search.set_reachability_query(source_set, target_set);
    for(auto i:result){
        cout<<i.first<<"->"<<i.second<<endl;
        EXPECT_TRUE(set_search.pll->reachability_query(i.first,i.second));
    }
    return ;
}