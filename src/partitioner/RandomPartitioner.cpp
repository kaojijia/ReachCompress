
#include<iostream>
#include<queue>
#include<vector>
#include<algorithm>
#include<random>
#include"partitioner/RandomPartitioner.h"

using namespace std;

void RandomPartitioner::partition(Graph &graph, PartitionManager &partition_manager)
{
    
    vector<int> nodes(graph.get_num_vertices()+10, -1);
    for(int i = 0; i<graph.vertices.size(); i++){
        if(graph.vertices[i].out_degree == 0 && graph.vertices[i].in_degree == 0) {
            continue;
        }
        nodes.push_back(i);
    }
    
    for(int i = nodes.size()-1; i >=0 ; i--){
        if(nodes[i] != -1){
            break;
        }
        nodes.pop_back();
    }

    // sort(nodes.begin(), nodes.end(),[&](int a, int b){
    //     int da = graph.vertices[a].out_degree + graph.vertices[a].in_degree;
    //     int db = graph.vertices[b].out_degree + graph.vertices[b].in_degree;
    //     return da < db;
    // });
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dis(1, num_partitions);

    for(auto node : nodes){
        int partition_id = dis(gen);
        graph.set_partition_id(node, partition_id); // 初始分区号为节点自身编号
    }

    // 建立分区图和对应的信息
    partition_manager.update_partition_connections();
    partition_manager.build_partition_graph();
    partition_manager.build_connections_graph();
}


