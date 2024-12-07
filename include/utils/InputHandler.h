#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "graph.h"
#include "IOHandler.h"
#include <string>

class InputHandler : public IOHandler{
public:
    explicit InputHandler(const std::string& input_file);
    
    // 读取图的边集并构建图
    void readGraph(Graph& g);

private:
    std::string input_file;  // 输入文件路径
};

#endif
