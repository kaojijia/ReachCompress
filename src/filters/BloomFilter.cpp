#include "BloomFilter.h"
#include "BidirectionalBFS.h"
#include <cmath>
#include <functional>

// 构造函数
BloomFilter::BloomFilter(Graph& graph)
    : graph(graph), numHashFunctions(3) { // 默认使用3个哈希函数
    size_t numVertices = graph.vertices.size(); // 获取顶点数量
    filters.resize(numVertices); // 修改为 numVertices
    insertedElements.resize(numVertices, 0); // 初始化每个顶点的插入计数
}

// 离线初始化
void BloomFilter::offline_industry() {
    BidirectionalBFS bfs(graph);
    size_t numVertices = graph.vertices.size(); // 修改为 graph.vertices().size()
    for (int u = 0; u < numVertices; ++u) {
        for (int v = 0; v < numVertices; ++v) {
            if (bfs.reachability_query(u, v)) {
                auto hashes = generateHashes(v);
                for (size_t h : hashes) {
                    filters[u].set(h); // 设置对应的位
                }
                insertedElements[u]++; // 插入计数
            }
        }
    }
}

// 计算假阳性率
double BloomFilter::false_positive_rate(int vertex) {
    if (vertex < 0 || vertex >= insertedElements.size()) {
        return -1.0; // 非法顶点返回 -1
    }
    size_t m = filters[vertex].size(); // 位数组大小
    size_t n = insertedElements[vertex]; // 插入的元素数量
    size_t k = numHashFunctions; // 哈希函数数量

    // 计算假阳性率公式
    double power = -static_cast<double>(k * n) / m;
    return std::pow(1 - std::exp(power), k);
}

// 可达性查询
bool BloomFilter::reachability_query(int source, int target) {
    if (source < 0 || source >= filters.size() || target < 0 || target >= filters.size()) {
        return false; // 非法索引返回不可达
    }
    auto hashes = generateHashes(target);
    for (size_t h : hashes) {
        if (!filters[source].test(h)) {
            return false; // 如果某一位没有被置1，则不可达
        }
    }
    return true; // 可能可达
}

// 计算索引大小
std::vector<std::pair<std::string, std::string>> BloomFilter::getIndexSizes() const {
    std::vector<std::pair<std::string, std::string>> indexSizes;

    // 每个 Bloom Filter 的大小（64 位 = 8 字节）
    size_t perFilterSize = sizeof(std::bitset<64>);

    // 总顶点数
    size_t numVertices = graph.vertices.size();

    // 计算整体过滤器大小
    size_t totalFilterSize = numVertices * perFilterSize;

    // 填充结果向量
    indexSizes.emplace_back("PerVertexFilterSize(bytes)", std::to_string(perFilterSize));
    indexSizes.emplace_back("TotalFilterSize(bytes)", std::to_string(totalFilterSize));
    indexSizes.emplace_back("NumberOfHashFunctions", std::to_string(numHashFunctions));

    return indexSizes;
}
// 哈希函数生成
std::vector<size_t> BloomFilter::generateHashes(int key) const {
    std::vector<size_t> hashes;
    std::hash<int> hasher;
    size_t seed = key;
    for (size_t i = 0; i < numHashFunctions; ++i) {
        seed = hasher(seed); // 每次生成一个新哈希值
        hashes.push_back(seed % 64); // 将哈希值映射到位数组大小范围
    }
    return hashes;
}