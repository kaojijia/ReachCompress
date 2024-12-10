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
#include "CSR.h"
#include <dirent.h>      // 用于目录遍历
#include <sys/types.h>   // 定义了DIR等数据类型
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

// 使用命名空间简化代码
using namespace std;

TEST(CSRTest, BASIC_TEST){
    string edgeFile2 = PROJECT_ROOT_DIR"/Edges/generate/test_SCC_1";
    Graph g2(true);
    InputHandler inputHandler2(edgeFile2);
    inputHandler2.readGraph(g2);
    g2.setFilename(edgeFile2);
    CSRGraph csr;
    csr.fromGraph(g2);
    csr.printAllInfo();
    auto i = csr.getNodesNum();
    csr.addEdge(4,6);
    csr.printAllInfo();
    csr.removeEdge(4,6);
    csr.printAllInfo();
    csr.removeNode(4);
    csr.printAllInfo();
    csr.printCSRs();

    cout<<"over"<<endl;
}