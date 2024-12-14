#pragma once

#include<iostream>
#include<cstdint>
#include <sstream>
#include <bitset>
#include <memory>
#include"Algorithm.h"
#include"graph.h"

using namespace std;

struct TreeNode{
    int tree_id;
    uint32_t min_postorder;
    uint32_t postorder;
    TreeNode* next = nullptr;
};

class TreeCover : public Algorithm {
public:
    TreeCover(Graph& graph): g(graph) {
    }
    ~TreeCover(){

    }
    void offline_industry() override;
    void printIndex();
    bool reachability_query(int source, int target) override;
    std::unordered_map<std::string, size_t> getIndexSizes() const override {
        // 示例实现，根据实际情况返回索引名称与大小的键值对
        std::unordered_map<std::string, size_t> index_sizes;
        index_sizes["tree_nodes"] = sizeof(TreeNode) * g.vertices.size();
        return index_sizes;
    }

private:
    Graph& g;
    TreeNode** tree_nodes;
    uint32_t post_traverse(uint32_t index, uint32_t tree_num, uint32_t& order);

};