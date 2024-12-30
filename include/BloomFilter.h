#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include "graph.h"
#include "Algorithm.h"
#include <vector>
#include <bitset>

class BloomFilter : public Algorithm {
public:
    BloomFilter(Graph& graph);
    void offline_industry() override;
    bool reachability_query(int source, int target) override;
    double false_positive_rate(int vertex); // 计算某个顶点的假阳性率
    std::vector<std::pair<std::string, std::string>> getIndexSizes() const override; // 计算索引大小

private:
    Graph& graph;
    std::vector<std::bitset<64>> filters;
    size_t numHashFunctions;
    std::vector<size_t> insertedElements; // 记录每个顶点插入的元素数量

    std::vector<size_t> generateHashes(int key) const;
};

#endif // BLOOM_FILTER_H
