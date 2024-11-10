// CommunityDetection.h
#ifndef COMMUNITY_DETECTION_H
#define COMMUNITY_DETECTION_H

#include "graph.h"
#include "Algorithm.h"

class CommunityDetection : public Algorithm{
public:
    explicit CommunityDetection(Graph& graph);

    void offline_industry() override;
    bool reachability_query(int source, int target) override;


    void louvainAlgorithm();
    void labelPropagation();
    // 其他社区检测算法的方法
private:
    Graph& graph_;
    // 其他必要的成员变量
};

#endif // COMMUNITY_DETECTION_H