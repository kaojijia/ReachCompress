#include <gtest/gtest.h>
#include "compression.h"
#include "graph.h"
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"


class CompressionTest : public ::testing::Test {
protected:
    Graph g;
    // Compression* com;

    // 使用项目根目录宏构建绝对路径
    std::string filename = "tweibo-edgelist_DAG";
    std::string gene_path = "generate/";
    std::string edge_path = "/Edges/";
    std::string result_path = "/result/";

    virtual void SetUp() {
        InputHandler inputHandler(PROJECT_ROOT_DIR + edge_path + filename);
        inputHandler.readGraph(g);
        // OutputHandler::printGraphInfo(g);  
    }

    virtual void TearDown() {

    }
};

// TEST_F(CompressionTest, ConstructRAG){

//     Compression com(g);
//     // com.compressRAG();
    
//     // OutputHandler::printGraphInfo(g);


// }

TEST_F(CompressionTest, DISABLED_MergeIn1Out1Nodes){
    Compression com(g);
    com.mergeIn1Out1Nodes();
    OutputHandler::printGraphInfo(com.getGraph());
    OutputHandler::printMapping(com);
}
TEST_F(CompressionTest, DISABLED_MergeNodes){
    auto i = g.statics();
    std::cout <<"nodes num:"<< i.first << " " <<"edges num:" <<i.second << std::endl;
    Compression com(g);
    com.del_nodes();
    com.mergeIn1Out1Nodes();
    com.mergeRouteNodes();
    // OutputHandler::printGraphInfo(com.getGraph());
    // OutputHandler::printMapping(com);
    OutputHandler out(result_path + filename + "_compressed");
    i = com.getGraph().statics();
    out.writeGraphInfo(com.getGraph());
    std::cout <<"nodesnum:"<< i.first << " " <<"edgesnum:" <<i.second << std::endl;
}

