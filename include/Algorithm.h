#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <unordered_map>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
using namespace std;

class Algorithm {
public:
    virtual void offline_industry() = 0;
    virtual bool reachability_query(int source, int target) = 0;
    // 声明测量索引大小的纯虚函数，返回索引名称与大小的键值对
    virtual std::unordered_map<std::string, size_t> getIndexSizes() const = 0;
    virtual ~Algorithm() = default;
    //输出当前时间
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
        tm local_tm;
    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_tm, &now_time_t);
    #else
        localtime_r(&now_time_t, &local_tm);
    #endif

        std::stringstream ss;
        ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
        return ss.str();
    }
};

#endif // ALGORIGHM_H