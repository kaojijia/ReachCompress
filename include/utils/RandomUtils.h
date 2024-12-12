#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <vector>
#include <random>
#include <algorithm>

class RandomUtils {
public:
    // 生成不重复的随机查询对
        // 生成不重复的随机查询对,容易死循环，因为要一直找不重复的但是找不到，推荐用下面那个，可能会重复但不会死循环
    static std::vector<std::pair<int, int>> generateUniqueQueryPairs(int num_queries, int max_value, unsigned int seed = std::random_device{}()) {
        std::vector<std::pair<int, int>> query_pairs;
        query_pairs.reserve(num_queries);
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(0, max_value - 1);

        while (query_pairs.size() < num_queries) {
            int source = dist(rng);
            int target = dist(rng);
            if (source != target && std::find(query_pairs.begin(), query_pairs.end(), std::make_pair(source, target)) == query_pairs.end()) {
                query_pairs.emplace_back(source, target);
            }
        }

        return query_pairs;
    }
        static std::vector<std::pair<int, int>> generateQueryPairs(int num_queries, int max_value, unsigned int seed) {
        std::vector<std::pair<int, int>> query_pairs;
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(0, max_value - 1);

        while (query_pairs.size() < num_queries) {
            int source = dist(rng);
            int target = dist(rng);
            if (source != target) {
                query_pairs.emplace_back(source, target);
            }
        }

        return query_pairs;
    }
};

#endif