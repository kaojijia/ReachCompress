#include "CompressedSearch.h"
#include "ReachRatio.h"
#include "GraphPartitioner.h"
#include "LouvainPartitioner.h" 
#include "InfomapPartitioner.h" 
#include "BloomFilter.h"
#include "NodeEmbedding.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <bits/unique_ptr.h>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <stack>


void CompressedSearch::set_partitioner(std::string partitioner_name)
{
    // 默认使用 Infomap 分区算法，可根据需要替换
    if (partitioner_name == "Louvain") {
        partitioner_ = std::unique_ptr<LouvainPartitioner>(new LouvainPartitioner());
    } else if (partitioner_name == "Infomap") {
        partitioner_ = std::unique_ptr<InfomapPartitioner>(new InfomapPartitioner());
        //partitioner_ = std::unique_ptr<GraphPartitioner>(new GraphPartitioner());
    } else {
        throw std::invalid_argument("Unsupported partitioner name");
    }
}

/**
 * @brief 离线索引建立，执行图分区并构建辅助数据结构。
 */
void CompressedSearch::offline_industry() {
    partition_graph();  ///< 执行图分区算法
    // 构建 Bloom Filter
    // bloom_filter_.build(g);
    // 构建 Node Embedding
    // node_embedding_.build(g);
    part_bfs = std::unique_ptr<BidirectionalBFS>(new BidirectionalBFS(partition_manager_.part_g));
}

void CompressedSearch::offline_industry(size_t num_vertices, float ratio)
{
    partition_graph();  ///< 执行图分区算法
    //建立每个分区的索引
    build_partition_index(ratio, num_vertices); 
    //建立分区相互联系的图
    part_bfs = std::unique_ptr<BidirectionalBFS>(new BidirectionalBFS(partition_manager_.part_g));
}

/**
 * @brief 在线查询，判断两个节点之间的可达性。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::reachability_query(int source, int target) {
    if(source == target) return true;
    if(source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0) {
        return false;
    }
    if(g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0 ) return false;
    
    // 使用 Bloom Filter 和 Node Embedding 进行快速判断
    // if (!bloom_filter_.possibly_connected(source, target)) {
    //     return false;
    // }
    // if (!node_embedding_.are_nodes_related(source, target)) {
    //     return false;
    // }


    int source_partition = g.get_partition_id(source);
    int target_partition = g.get_partition_id(target);

    if (source_partition == target_partition) {
        bool result;
        if(this->is_index){
            result = query_index_within_partition(source, target, source_partition);  ///< 同分区带索引查询
        }
        else{
            result = query_within_partition(source, target);  ///< 同分区查询
        }
        if(result) return true;
        return bfs.reachability_query(source, target); //全图找
    }else if (source_partition == -1 || target_partition == -1) {
        return false;
    }else {
        return query_across_partitions(source, target); ///< 跨分区查询
    }
}


/**
 * @brief 执行图分区算法。
 */
void CompressedSearch::partition_graph() {
    partitioner_->partition(g, partition_manager_);
}

/**
 * @brief 在同一分区内进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_within_partition(int source, int target) {
    if(source == target) return true;
    if(source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0) {
        return false;
    }
    if(g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0 ) return false;
    
    if(g.get_partition_id(source) != g.get_partition_id(target)) {
        //cout<< "输入节点  "<<source<<"  "<<target<<"  不在同一分区内，无法进行可达性查询"<<endl;
        return false;
    }
    int partition_id = g.get_partition_id(source);
    auto result = bfs.findPath(source, target, partition_id);
    if(result.size() > 0) {
        return true;
    }
    return false;
}

bool CompressedSearch::query_index_within_partition(int source, int target, int partition_id)
{

    //先过滤是否可达
    if(source == target) return true;
    auto subgraph = partition_manager_.partition_subgraphs[partition_id];
    if(g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0 ) return false;
    if(source >= subgraph.vertices.size() || target >= subgraph.vertices.size() || source < 0 || target < 0) {
        return false;
    }
    if(g.get_partition_id(source) != g.get_partition_id(target)) {
        //cout<< "输入节点  "<<source<<"  "<<target<<"  不在同一分区内，无法进行可达性查询"<<endl;
        return false;
    }


    //在三个索引中查询
    if (subgraph.get_num_vertices() < this->num_vertices) {
        // 使用 small_index_ 进行查询
        auto& node_to_index = small_mapping_[partition_id];
        if (node_to_index.find(source) == node_to_index.end() || node_to_index.find(target) == node_to_index.end()) {
            return false;
        }
        int mapped_source = node_to_index[source];
        int mapped_target = node_to_index[target];
        return small_index_[partition_id][mapped_source][mapped_target].test(0);
    } else if (compute_reach_ratio(subgraph) < this->ratio) {
        // 使用 PLL 进行查询
        return pll_index_[partition_id]->reachability_query(source, target);
    } else {
        // 使用 unreachable_index_ 进行查询
        auto& node_to_index = unreachable_mapping_[partition_id];
        if (node_to_index.find(source) == node_to_index.end() || node_to_index.find(target) == node_to_index.end()) {
            return false;
        }
        int mapped_source = node_to_index[source];
        int mapped_target = node_to_index[target];
        for(auto& edge : unreachable_index_[partition_id][mapped_source]){
            if(mapped_target == edge) return false;
        }
        return true;
    }

    return false;
}

/**
 * @brief 在不同分区间进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_across_partitions(int source, int target) {
    auto source_partition = g.get_partition_id(source);
    auto target_partition = g.get_partition_id(target);
    // 找分区的路径
    std::vector<int> path = part_bfs->findPath(source_partition,target_partition);
    if(path.empty())return false;
    auto edges = partition_manager_.get_partition_adjacency(path[0], path[1]);
    return dfs_partition_search(source, edges.original_edges, path, target);
}

/**
 * @brief 辅助方法，执行分区间的迭代式DFS搜索。
 * @param current_partition 当前分区ID。
 * @param target_partition 目标分区ID。
 * @param visited 已访问的分区集合。
 * @param path 从当前节点出发的可达路径集合
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::dfs_partition_search(int u, std::vector<std::pair<int, int>>edges, std::vector<int>path, int target) {
    auto current_partition = path[0];
    if (current_partition != g.get_partition_id(u)) {
        throw std::logic_error("Current partition from path[] does not match the partition of node u");
    }
    for(auto &edge:edges){
        //上一个前序点无法到达这条边就下一轮循环
        if(is_index){
            if(!query_index_within_partition(u,edge.first,current_partition)) continue;
        }else{
            if(!query_within_partition(u,edge.first)) continue;
        }
        //如果下一个分区是终点分区就使用BFS查找，如果不是指向终点分区就继续递归找
        auto next_partition = path[1];
        if(g.get_partition_id(target) == next_partition){
            if(is_index){
                return query_index_within_partition(edge.second, target, next_partition);
            }
            return query_within_partition(edge.second, target);
        }
        else{
            std::vector<int> remain_path(path.begin() + 1, path.end());
            auto next_edges = partition_manager_.get_partition_adjacency(path[0],path[1]);
            return dfs_partition_search(edge.second,next_edges.original_edges,remain_path,target);
        }
    }

    return false;
}

void CompressedSearch::build_partition_index(float ratio, size_t num_vertices)
{
    #ifdef DEBUG
    std::cout << "Building partition index..." << std::endl;
    #endif
    
    // 打印 mapping 信息
    std::cout << "Partition to Node Mapping:" << std::endl;
    for (const auto& partition_pair : partition_manager_.mapping) {
        std::cout << "Partition " << partition_pair.first << ": ";
        for (const auto& node : partition_pair.second) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
    this->ratio = ratio;
    this->num_vertices = num_vertices;
    for (auto &subgraph :this->partition_manager_.partition_subgraphs){
        if(subgraph.first == -1)continue;
        //计算分区的比例信息
        float total_ratio = compute_reach_ratio(subgraph.second);
        auto partition_id = subgraph.first;

        if (subgraph.second.get_num_vertices()==1)
        {
            //一个点就不管了
            continue;
        }
        //根据情况构建索引,如果点数小于100那么就要建立一个子图的邻接矩阵
        //如果g.get_num_vertices点数大于传进来的num_vertices,看分区图的ratio
        //ratio < 传进来的ratio,那么就建立PLL索引
        //ratio >= 传进来的ratio,那么就建立不可达的邻接表,对子图做V*V的循环,每个里面调用一次双边BFS,如果不可达就记录下来
        //建立索引的方案是,在CompressSearch中建立三个数据结构
        if(subgraph.second.get_num_vertices() < num_vertices) {
            // 构建可达索引:邻接矩阵
            //映射后的号
            size_t index = 0;
            // 第一位是源节点，第二位是索引中的序号
            std::unordered_map<size_t, size_t> node_to_index;
            for(size_t i = 0; i < subgraph.second.vertices.size(); i++) {
                if(subgraph.second.vertices[i].in_degree > 0 || subgraph.second.vertices[i].out_degree > 0) {
                    node_to_index[i] = index++;
                }
            }

            small_mapping_[subgraph.first] = node_to_index;
            small_index_[subgraph.first] = std::vector<std::vector<std::bitset<1>>>(index, std::vector<std::bitset<1>>(index));

            // 初始化为全 0（实际上已经是全 0，不需要额外操作）
            for (size_t u = 0; u < subgraph.second.vertices.size(); u++) {
                if(subgraph.second.vertices[u].in_degree == 0 && subgraph.second.vertices[u].out_degree == 0) continue;
                for (size_t v = 0; v < subgraph.second.vertices.size(); v++) {
                    if(subgraph.second.vertices[v].in_degree == 0 && subgraph.second.vertices[v].out_degree == 0) continue;
                    if (u == v) continue;
                    if (this->query_within_partition(u, v)) {
                        int mapped_u = node_to_index[u];
                        int mapped_v = node_to_index[v];
                        
                        small_index_[subgraph.first][mapped_u][mapped_v].set(0); // 设置为 1 表示可达
                    }
                }
            }
            // 打印二维数组
            std::cout<<"small_index_:"<<std::endl;
            for (size_t i = 0; i < small_index_[subgraph.first].size(); i++) {
                for (size_t j = 0; j < small_index_[subgraph.first][i].size(); j++) {
                    std::cout << small_index_[subgraph.first][i][j] << " ";
                }
                std::cout << std::endl;
            }
        } else if(total_ratio < ratio) {
            //构建pll
            pll_index_[subgraph.first] = new PLL(subgraph.second);
            pll_index_[subgraph.first]->offline_industry();
            // 打印 PLL 的 IN 和 OUT 集合
            
            std::cout << "PLL IN and OUT sets for partition " << subgraph.first << ":" << std::endl;
            for (size_t i = 0; i < pll_index_[subgraph.first]->IN.size(); ++i) {
                if(pll_index_[subgraph.first]->IN[i].size()>0){
                    std::cout << "Node " << i << " IN: ";
                    for (int in_node : pll_index_[subgraph.first]->IN[i]) {
                        std::cout << in_node << " ";
                    }
                    std::cout << std::endl;
                }

            }

            for (size_t i = 0; i < pll_index_[subgraph.first]->OUT.size(); ++i) {
                if(pll_index_[subgraph.first]->OUT[i].empty())continue;
                std::cout << "Node " << i << " OUT: ";
                for (int out_node : pll_index_[subgraph.first]->OUT[i]) {
                    std::cout << out_node << " ";
                }
                std::cout << std::endl;
            }
        } else {
            // 构建不可达的邻接表
            // 初始化映射和不可达邻接表
            std::unordered_map<size_t, size_t>& node_to_index = unreachable_mapping_[partition_id];
            std::vector<std::vector<size_t>>& unreachable_adj = unreachable_index_[partition_id];
            size_t current_index = 0;

            // 遍历子图中的所有节点对，记录不可达的点对
            for(size_t u = 0 ; u < subgraph.second.vertices.size(); u++){
                if(subgraph.second.vertices[u].in_degree == 0 && subgraph.second.vertices[u].out_degree == 0) continue;
                for(size_t v = 0 ; v < subgraph.second.vertices.size(); v++){
                    if(subgraph.second.vertices[v].in_degree == 0 && subgraph.second.vertices[v].out_degree == 0) continue;
                    if(u == v) continue;
                    bool result = this->query_within_partition(u, v);
                    if(!result){

                        // 只在遇到不可达点对时进行映射
                        // 如果source未映射，则映射并扩展不可达邻接表
                        if (node_to_index.find(u) == node_to_index.end()) {
                            node_to_index[u] = current_index++;
                            unreachable_adj.emplace_back(); // 添加一个新的邻接列表
                        }

                        // 如果target未映射，则映射
                        if (node_to_index.find(v) == node_to_index.end()) {
                            node_to_index[v] = current_index++;
                            unreachable_adj.emplace_back(); // 添加一个新的邻接列表
                        }

                        size_t mapped_u = node_to_index[u];
                        size_t mapped_v = node_to_index[v];
                        unreachable_adj[mapped_u].push_back(mapped_v);
                    }
                }
            }
            // 打印不可达邻接表
            std::cout << "Unreachable Adjacency List "<<std::endl;
            for (size_t i = 0; i < unreachable_adj.size(); i++) {
                std::cout << "Node " << i << ": ";
                for (size_t j = 0; j < unreachable_adj[i].size(); j++) {
                    std::cout << unreachable_adj[i][j] << " ";
                }
                std::cout << std::endl;
            }
        }   

    }



}