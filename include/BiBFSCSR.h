#ifndef BiBFSCSR_H
#define BiBFSCSR_H

#include "graph.h"
#include "CSR.h"
#include "Algorithm.h"
#include <vector>
#include <queue>
#include <unordered_set>

class BiBFSCSR : public Algorithm {
public:
    BiBFSCSR(Graph& graph);

    // bool reachability(int source, int target) override;

    void offline_industry() override{}

    bool reachability_query(int source, int target) override;

    // 找路径是否可达，如果可达返回路径，否则返回空。第三个参数，分区内搜索的时候要设置成true
    std::vector<int> findPath(int source, int target, int partition_number = -1); 
    std::vector<std::pair<std::string, std::string>> getIndexSizes() const override {
        return {{"G'CSR", std::to_string(csr->getMemoryUsage())}};
    }

private:
    Graph& g;
    CSRGraph* csr;

};

#endif  // BiBFSCSR_H
