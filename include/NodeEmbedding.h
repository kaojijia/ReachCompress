
#ifndef NODE_EMBEDDING_H
#define NODE_EMBEDDING_H

#include "graph.h"
#include <unordered_map>

class NodeEmbedding {
public:
    void build(const Graph& graph);
    bool are_nodes_related(int source, int target) const;
private:
    std::unordered_map<int, std::vector<float>> embeddings_;
    // ...existing code...
};

#endif // NODE_EMBEDDING_H