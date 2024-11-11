
#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include "graph.h"
#include <vector>

class BloomFilter {
public:
    void build(const Graph& graph);
    bool possibly_connected(int source, int target) const;
private:
    std::vector<bool> bitArray_;
    // ...existing code...
};

#endif // BLOOM_FILTER_H