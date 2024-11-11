// ...existing code...

#include "CompressedSearch.h"

// ...existing code...

/**
 * @brief 测试 CompressedSearch 类的可达性查询功能。
 */
void test_compressed_search() {
    Graph graph;
    // ...构建图...

    CompressedSearch cs(graph);
    cs.offline_industry();

    bool result = cs.reachability_query(1, 5);
    // ...existing code...
}

// ...existing code...
