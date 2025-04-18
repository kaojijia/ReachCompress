#ifndef PLL_H
#define PLL_H

#include "graph.h"
#include "CSR.h"
#include "Algorithm.h"
#include <vector>
#include <set>

class PLL : public Algorithm
{
public:
    PLL(Graph &graph); // 构造函数，传入图的引用
    ~PLL();

    // IN和OUT集合的有序插入
    void insertSorted(std::vector<int> &labelVec, int val);

    void offline_industry() override;
    bool reachability_query(int source, int target) override;

    // 构建PLL标签
    void buildPLLLabels();
    // 完全体标签
    void buildPLLLabelsUnpruned();
    // 可达性查询
    bool query(int u, int v);
    // 用构造的数组做可达性查询
    bool queryinArray(int source, int target);

    std::vector<std::vector<int>> IN;
    std::vector<std::vector<int>> OUT;



    // std::unordered_map<std::string, size_t> getIndexSizes() const override {
    //     size_t inSize = 0;
    //     size_t outSize = 0;
    //     for (const auto& inSet : IN) {
    //         inSize += inSet.size();
    //     }
    //     for (const auto& outSet : OUT) {
    //         outSize += outSet.size();
    //     }
    //     // 假设每个整数占用 4 字节
    //     inSize *= sizeof(int);
    //     outSize *= sizeof(int);
    //     return {{"IN", inSize}, {"OUT", outSize}};
    // }

    std::vector<std::pair<std::string, std::string>> getIndexSizes() const override
    {
        size_t inPointersSize = 0;
        size_t outPointersSize = 0;
        size_t inSetsSize = 0;
        size_t outSetsSize = 0;

        // 使用 pointer_length 来计算 in_pointers 和 out_pointers 的大小
        inPointersSize = pointer_length * sizeof(uint32_t);
        outPointersSize = pointer_length * sizeof(uint32_t);

        // 使用 in_sets_length 和 out_sets_length 来计算 in_sets 和 out_sets 的大小
        inSetsSize = in_sets_length * sizeof(uint32_t);
        outSetsSize = out_sets_length * sizeof(uint32_t);

        std::vector<std::pair<std::string, std::string>> index_sizes;
        index_sizes.emplace_back("in_pointers", std::to_string(inPointersSize));
        index_sizes.emplace_back("out_pointers", std::to_string(outPointersSize));
        index_sizes.emplace_back("in_sets", std::to_string(inSetsSize));
        index_sizes.emplace_back("out_sets", std::to_string(outSetsSize));

        return index_sizes;
    }

private:
    Graph &g; // 引用图

    std::vector<std::vector<int>> adjList;        // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList; // 逆邻接表

    uint32_t *in_pointers;
    uint32_t *out_pointers;
    uint32_t *in_sets;
    uint32_t *out_sets;

    // pointer的长度
    uint32_t pointer_length;

    // in和out sets的长度
    uint32_t in_sets_length;
    uint32_t out_sets_length;

    void buildAdjList();
    void buildInOut();

    // 2hop可达性查询，用于剪枝
    bool HopQuery(int u, int v);

    // IN和OUT集合去重
    void simplifyInOutSets();

    void bfsUnpruned(int start, bool is_reversed);
    void bfsPruned(int start);

    void add_self();

    // 确定剪枝顺序
    std::vector<int> orderByDegree();

    // 转换成类CSR格式用于查询
    bool convertToArray();
};

#endif
