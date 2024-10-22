#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

#include <string>
#include <vector>

class OutputHandler {
public:
    explicit OutputHandler(const std::string& output_file);
    
    // 写入2-hop覆盖信息
    void writeCoverInfo(const std::vector<int>& cover);

    // 写入可达性查询信息
    void writeReachabilityQuery(int u, int v, bool result);

private:
    std::string output_file;  // 输出文件路径
};

#endif
