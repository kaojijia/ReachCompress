#include <gtest/gtest.h>
#include "compression.h"
#include "graph.h"
#include "utils/InputHandler.h"
#include "utils/OutputHandler.h"

TEST(CompressionTest, ConstructRAG){
    Graph g(true);

    // 使用项目根目录宏构建绝对路径
    std::string filename = "gene_edges_20241029_135003";
    std::string gene_path = "/Edges/generate/";
    std::string result_path = "/result/";
    
    
    InputHandler inputHandler(PROJECT_ROOT_DIR + gene_path + filename);
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);  

    Compression com(g);
    com.compressRAG();
    
    OutputHandler::printGraphInfo(g);


}

TEST(CompressionTest, MergeNodes){
    Graph g(true);

    // 使用项目根目录宏构建绝对路径
    std::string filename = "gene_edges_20241029_135003";
    std::string gene_path = "/Edges/generate/";
    std::string result_path = "/result/";
    
    InputHandler inputHandler(PROJECT_ROOT_DIR + gene_path + filename);
    inputHandler.readGraph(g);
    OutputHandler::printGraphInfo(g);  

    Compression com(g);

    

    
}