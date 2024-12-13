#ifndef PLL_H
#define PLL_H

#include "graph.h"
#include "CSR.h"
#include "Algorithm.h"
#include <vector>


class PLL : public Algorithm{
public:
    PLL(Graph& graph);  // 构造函数，传入图的引用

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

    
    std::unordered_map<std::string, size_t> getIndexSizes() const override{
        size_t inSize = 0;
        size_t outSize = 0;
        for (const auto& inSet : IN) {
            inSize += inSet.size();
        }
        for (const auto& outSet : OUT) {
            outSize += outSet.size();
        }
        return {{"IN", inSize}, {"OUT", outSize}};
    }
private:
    Graph& g;  // 引用图

    std::vector<std::vector<int>> adjList;         // 正向邻接表
    std::vector<std::vector<int>> reverseAdjList;  // 逆邻接表

    uint32_t* in_pointers;
    uint32_t* out_pointers;
    uint32_t* in_sets;
    uint32_t* out_sets;

    void buildAdjList();
    void buildInOut();
    
    //2hop可达性查询，用于剪枝
    bool HopQuery(int u, int v);

    //IN和OUT集合去重
    void simplifyInOutSets();
    
    void bfsUnpruned(int start, bool is_reversed);
    void bfsPruned(int start);

    //确定剪枝顺序    
    std::vector<int> orderByDegree();


    //转换成类CSR格式用于查询
    bool convertToArray();
};

#endif

