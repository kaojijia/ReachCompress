#include "gtest/gtest.h"
#include "graph.h"
#include "SetSearch.h"
#include <algorithm>  // 确保包含算法库
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"


class GraphTest : public ::testing::Test {
protected:
    Graph g;

    // 使用项目根目录宏构建绝对路径
    std::string filename = "test_SCC_1";
    std::string gene_path = "/generate/";
    std::string edge_path = "/Edges/";
    std::string result_path = "/result/";

    virtual void SetUp() {
        InputHandler inputHandler(PROJECT_ROOT_DIR + edge_path + filename);
        inputHandler.readGraph(g);
        OutputHandler::printGraphInfo(g);  
    }

    virtual void TearDown() {

    }
};


// TEST_F(GraphTest, DISABLED_RemoveTest) {
    
//     g.removeNode(2);
//     // g.removeEdge(2,3);
//     OutputHandler::printGraphInfo(g);

// }

TEST_F(GraphTest, setTest) {
    Graph g;
    g.addEdge(0,1);
    SetSearch set_search(g);

}