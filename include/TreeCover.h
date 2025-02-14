#pragma once

#include <iostream>
#include <cstdint>
#include <sstream>
#include <bitset>
#include <memory>
#include "Algorithm.h"
#include "graph.h"

using namespace std;

// 存两个值：在当前树上的后序遍历值和最小的后序遍历值
// 看两个点的区间是否互相包含来确认是否可达

struct TreeNode
{
    int tree_id;//代表是哪棵树，一个图中可能有多个生成树，一个节点可能在多个生成树中都有值
    uint32_t min_postorder;
    uint32_t postorder;
    TreeNode *next = nullptr; //没用
};

class TreeCover : public Algorithm
{
public:
    TreeCover(Graph &graph) : g(graph)
    {
    }
    ~TreeCover()
    {
        if (tree_nodes != nullptr)
        {
            for (uint32_t i = 0; i < g.vertices.size(); ++i)
            {
                delete tree_nodes[i];
            }
            delete[] tree_nodes;
        }
    }
    void offline_industry() override;
    void printIndex();
    bool reachability_query(int source, int target) override;
    std::vector<std::pair<std::string, std::string>> getIndexSizes() const override
    {
        std::vector<std::pair<std::string, std::string>> index_sizes;
        index_sizes.emplace_back("tree_nodes", std::to_string(sizeof(TreeNode) * g.vertices.size()));
        return index_sizes;
    }

private:
    Graph &g;
    TreeNode **tree_nodes;
    uint32_t post_traverse(uint32_t index, uint32_t tree_num, uint32_t &order);
};