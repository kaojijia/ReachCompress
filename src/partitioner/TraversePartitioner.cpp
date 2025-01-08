#include "partitioner/TraversePartitioner.h"
#include "graph.h"
#include "CSR.h"
#include "BiBFSCSR.h"
#include <iostream>
#include <vector>
#include <stack>
#include <utility>
#include <unordered_set>
#include <bitset>
// #define u32 uint32_t
using namespace std;

const int max_depth = 3; // 最大深度


void cal_reach_ratio(){};

void TraversePartitioner::partition(Graph& graph, PartitionManager& partition_manager)
{
    this->bibfs_ = make_shared<BiBFSCSR>(graph);
    this->csr_ = bibfs_->getCSR();
    // this->graph_ = &graph;

    //记录每个节点的分区
    partitions.resize(csr_->max_node_id, -1);
    //用来访问的节点
    vector<int> nodes(csr_->num_nodes, 0);
    //已经分好区的节点
    vector<u32> partition_visited(csr_->max_node_id+1, 0);
    for(int i = 0,count = 0; i < csr_->max_node_id+1; i++){
        if(csr_->getOutDegree(i) == 0 && csr_->getInDegree(i) == 0) continue;
        partitions[i] = i;
        nodes[count++] = i;
    } 

    //按照度降序排序
    sort(nodes.begin(), nodes.end(), [&](int a, int b) {
        return csr_->getOutDegree(a)+csr_->getInDegree(a) > csr_->getOutDegree(b)+csr_->getInDegree(b);
    });

    //遍历每个节点
    for(auto node : nodes){
        auto visited = dfs(max_depth, node);
        cout << "Partition " << node << " has nodes: ";
        for(auto v : visited){
            // graph.vertices[v].partition_id = node;
            cout<< v << " ";
        }
        cout << endl;
    }
    cout<<endl;
}



vector<u32> TraversePartitioner::dfs(
    u32 depth, 
    u32 vertex, 
    map<u32, PartitionReachable> *partition_reachable
    //分区和信息的记录

){
    stack<pair<u32,u32>> stack;
    stack.push(make_pair(vertex,0));
    unordered_set<u32> visited; 
    vector<u32> result;

    while(!stack.empty()){
        pair<u32,u32> p = stack.top();
        stack.pop();
        auto node = p.first;
        auto cur_depth = p.second;
        if(cur_depth > depth) continue;
        if(visited.find(node) != visited.end()) continue; // 如果节点已访问，跳过
        visited.insert(node); // 标记节点为已访问
        result.push_back(node);

        //如果要找是否有可达分区的话，在这里存入的分区号和可达信息
        if(partition_reachable != nullptr ){
            if(csr_->getPartition(node) == csr_->getPartition(vertex))continue;
            if(bibfs_->reachability_query(vertex, node)){
                (*partition_reachable)[node].out = true;
            }
            if(bibfs_->reachability_query(node, vertex)){
                (*partition_reachable)[node].in = true;
            }
        }

        u32 degree;
        auto out_neighbors = csr_->getOutgoingEdges(node, degree);
        for(auto i = 0; i < degree; i++){
            stack.push(make_pair(out_neighbors[i], cur_depth + 1));
        }

        degree = 0;
        auto in_neighbors = csr_->getIncomingEdges(node, degree);
        for(auto i = 0; i < degree; i++){
            stack.push(make_pair(in_neighbors[i], cur_depth + 1));
        }
    }

    return result;
}