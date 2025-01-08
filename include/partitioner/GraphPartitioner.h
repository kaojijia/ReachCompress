#ifndef GRAPH_PARTITIONER_H
#define GRAPH_PARTITIONER_H


#include "graph.h"
#include "PartitionManager.h"
#include "BidirectionalBFS.h"
#include <vector>
#include <memory>
#include <stack>
#include <utility>
#include <unordered_set>
using namespace std;

/**
 * @brief in:分区点是否可以到达本身,out:是否可以到达分区点
 * 
 */
struct PartitionReachable
{
    bool in = false; // 分区点是否可以到达本身
    bool out = false; // 是否可以经由边到达分区点
};


/**
 * @class GraphPartitioner
 * @brief 抽象基类，用于实现不同的图分区算法。
 */
class GraphPartitioner {
public:
    /**
     * @brief 虚析构函数。
     */
    virtual ~GraphPartitioner() = default;

    /**
     * @brief 执行图分区算法。
     * @param graph 需要分区的图。
     * @param partition_manager 用于管理和存储分区信息的管理器。
     */
    virtual void partition(Graph& graph, PartitionManager& partition_manager) = 0;


    vector<map<int, PartitionReachable>> get_reachable_partitions(Graph& graph, int max_depth){
        BidirectionalBFS bibfs(graph);
        vector<map<int, PartitionReachable>> partition_reachable(graph.vertices.size(), map<int, PartitionReachable>());


        unordered_set<int> partitions;
        for(int i = 0; i < graph.vertices.size(); i++){
            if(graph.vertices[i].out_degree == 0 && graph.vertices[i].in_degree == 0) continue;
            partitions.insert(graph.vertices[i].partition_id);
        }

        for(int i = 0; i < graph.vertices.size(); i++){
            int vertex = i;
            if(graph.vertices[i].out_degree == 0 && graph.vertices[i].in_degree == 0) continue;
            if(graph.vertices[i].partition_id == -1) continue;


            stack<pair<int,int>> stack;
            stack.push(make_pair(i,0));
            unordered_set<int> visited; 
            vector<int> result;

            while(!stack.empty()){
                pair<int,int> p = stack.top();
                stack.pop();
                auto node = p.first;
                auto cur_depth = p.second;
                if(cur_depth > max_depth) continue;
                
                if(visited.find(node) != visited.end()) continue; // 如果节点已访问，跳过
                
                visited.insert(node); // 标记节点为已访问

                result.push_back(node);
                //标记分区可达性
                if(node != graph.vertices[vertex].partition_id && partitions.find(node) != partitions.end()){
                    if(!partition_reachable[vertex][node].out  && bibfs.reachability_query(vertex, node)){
                        partition_reachable[vertex][node].out = true;
                    }
                    if(!partition_reachable[vertex][node].in && bibfs.reachability_query(node, vertex)){
                        partition_reachable[vertex][node].in = true;
                    }
                }
                for(auto neighbor : graph.vertices[node].LOUT){
                    stack.push(make_pair(neighbor, cur_depth + 1));
                }
                for(auto neighbor : graph.vertices[node].LIN){
                    stack.push(make_pair(neighbor, cur_depth + 1));
                }
            }
        }
        return partition_reachable;
    }
    

};

#endif // GRAPH_PARTITIONER_H