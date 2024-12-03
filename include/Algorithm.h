#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <unordered_map>
#include <string>

class Algorithm {
public:
    virtual void offline_industry() = 0;
    virtual bool reachability_query(int source, int target) = 0;
    // 声明测量索引大小的纯虚函数，返回索引名称与大小的键值对
    virtual std::unordered_map<std::string, size_t> getIndexSizes() const = 0;
    virtual ~Algorithm() = default;
};

#endif // ALGORIGHM_H