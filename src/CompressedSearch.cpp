#include "CompressedSearch.h"
#include "ReachRatio.h"
#include "GraphPartitioner.h"
#include "LouvainPartitioner.h"
#include "ImportPartitioner.h"
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
    this->partitioner_name_ = partitioner_name;
    // 默认使用 Infomap 分区算法，可根据需要替换
    if (partitioner_name == "Louvain")
    {
        partitioner_ = std::unique_ptr<LouvainPartitioner>(new LouvainPartitioner());
    }
    else if (partitioner_name == "Import")
    {
        partitioner_ = std::unique_ptr<ImportPartitioner>(new ImportPartitioner());
        // partitioner_ = std::unique_ptr<GraphPartitioner>(new GraphPartitioner());
    }
    else
    {
        throw std::invalid_argument("Unsupported partitioner name");
    }
}

/**
 * @brief 离线索引建立，执行图分区并构建辅助数据结构。
 */
void CompressedSearch::offline_industry()
{
    partition_graph(); ///< 执行图分区算法
    // 构建 Bloom Filter
    // bloom_filter_.build(g);
    // 构建 Node Embedding
    // node_embedding_.build(g);
    part_bfs_csr = std::unique_ptr<BiBFSCSR>(new BiBFSCSR(partition_manager_.part_g));
}

void CompressedSearch::offline_industry(size_t num_vertices, float ratio, string mapping_file)
{

    // TODO：执行压缩

    if (mapping_file != "")
        partition_manager_.read_equivalance_info(mapping_file); ///< 读取等价信息

    construct_filter(ratio); ///< 构建过滤器

    partition_graph(); ///< 执行图分区算法

    build_partition_index(ratio, num_vertices);

    // 建立分区相互联系的图
    part_bfs = std::unique_ptr<BidirectionalBFS>(new BidirectionalBFS(partition_manager_.part_g));
    part_bfs_csr = std::unique_ptr<BiBFSCSR>(new BiBFSCSR(partition_manager_.part_g));
}

void CompressedSearch::construct_filter(float ratio)
{
    float ratio_ = compute_reach_ratio(g);
    if (ratio_ > ratio)
    {
        // 稠密：不可达索引
        filter = std::unique_ptr<BloomFilter>(new BloomFilter(g));
        filter_name_ = "unreachable";
    }
    else
    {
        // 稀疏：可达索引
        filter = std::unique_ptr<TreeCover>(new TreeCover(g));
        filter_name_ = "reachable";
    }
    filter->offline_industry();
}

/**
 * @brief 在线查询，判断两个节点之间的可达性。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::reachability_query(int origin_source, int origin_target)
{
    cout << getCurrentTimestamp() << "开始查询" << origin_source << "to" << origin_target << endl;
    if (origin_source == origin_target)
        return true;
    uint32_t target = partition_manager_.get_equivalance_mapping(origin_target);
    uint32_t source = partition_manager_.get_equivalance_mapping(origin_source);
    if (source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0)
    {
        return false;
    }
    if (g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0)
        return false;


    // 使用filter快速判断
    if (filter_name_ != "")
    {
        bool filter_result = filter->reachability_query(source, target);
        if (filter_name_ == "unreachable")
        {
            if (filter_result == false)
            {
                return false;
            }
        }
        else if (filter_name_ == "reachable")
        {
            if (filter_result == true)
            {
                return true;
            }
        }
    }

    bool result = false;
    int source_partition = g.get_partition_id(source);
    int target_partition = g.get_partition_id(target);
    cout << getCurrentTimestamp() << "分区确定" << source_partition << "  " << target_partition << endl;
    if (source_partition == target_partition)
    {

        if (this->is_index)
        {
            result = query_index_within_partition(source, target, source_partition); ///< 同分区带索引查询
        }
        else
        {
            result = query_within_partition(source, target); ///< 同分区查询
        }
        return result;
        // return bfs.reachability_query(source, target); //全图找
    }
    else if (source_partition == -1 || target_partition == -1)
    {
        return false;
    }
    else
    {
        // result =  query_across_partitions(source, target); ///< 跨分区查询
        result = query_across_partitions_with_all_paths(source, target); ///< 跨分区查询
        return result;
    }
}

/**
 * @brief 执行图分区算法。
 */
void CompressedSearch::partition_graph()
{
    partitioner_->partition(g, partition_manager_);
}

/**
 * @brief 在同一分区内进行可达性查询。
 * @param source 源节点。
 * @param target 目标节点。
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::query_within_partition(int source, int target)
{
    if (source == target)
        return true;
    if (source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0)
    {
        return false;
    }
    if (g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0)
        return false;

    if (g.get_partition_id(source) != g.get_partition_id(target))
    {
        // cout<< "输入节点  "<<source<<"  "<<target<<"  不在同一分区内，无法进行可达性查询"<<endl;
        return false;
    }
    int partition_id = g.get_partition_id(source);
    auto result = bfs.findPath(source, target, partition_id);
    if (result.size() > 0)
    {
        return true;
    }
    return false;
}

bool CompressedSearch::query_index_within_partition(int source, int target, int partition_id)
{

    cout << getCurrentTimestamp() + "在" << partition_id << "分区内查询" << source << "到" << target << endl;
    // 先过滤是否可达
    if (source == target)
        return true;
    if (g.vertices[source].out_degree == 0 || g.vertices[target].in_degree == 0)
        return false;
    auto &subgraph = partition_manager_.partition_subgraphs[partition_id];
    // if(source >= subgraph.vertices.size() || target >= subgraph.vertices.size() || source < 0 || target < 0) {
    //     return false;
    // }
    if (g.get_partition_id(source) != g.get_partition_id(target))
    {
        return false;
    }
    cout << getCurrentTimestamp() << "    start searching in index" << endl;
    // 在三个索引中查询
    if (subgraph.get_num_vertices() < this->num_vertices)
    {
        cout << getCurrentTimestamp() + "开始查询小索引" << endl;
        // 使用 small_index_ 进行查询
        auto &node_to_index = small_mapping_[partition_id];
        if (node_to_index.find(source) == node_to_index.end() || node_to_index.find(target) == node_to_index.end())
        {
            cout << getCurrentTimestamp() + "越界" << endl;
            return false;
        }
        int mapped_source = node_to_index[source];
        int mapped_target = node_to_index[target];
        bool result = small_index_[partition_id][mapped_source][mapped_target].test(0);
        cout << getCurrentTimestamp() + "汇报结果" << endl;
        return result;
    }
    else if (subgraph.get_ratio() < this->ratio)
    {
        // 使用 PLL 进行查询
        cout << getCurrentTimestamp() << "  开始查询pll索引" << endl;
        bool result = pll_index_[partition_id]->reachability_query(source, target);
        cout << getCurrentTimestamp() << "  pll索引查询完成" << endl;
        return result;
    }
    else
    {
        // 使用 unreachable_index_ 进行查询
        cout << getCurrentTimestamp() + "开始查询不可达索引" << endl;
        auto &node_to_index = unreachable_mapping_[partition_id];
        if (node_to_index.find(source) == node_to_index.end())
        {
            return true;
        }
        int mapped_source = node_to_index[source];
        int mapped_target = node_to_index[target];
        for (auto &edge : unreachable_index_[partition_id][mapped_source])
        {
            if (mapped_target == edge)
                return false;
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
bool CompressedSearch::query_across_partitions(int source, int target)
{
    auto source_partition = g.get_partition_id(source);
    auto target_partition = g.get_partition_id(target);
    // 找分区的路径
    // TODO：这个路径不行就换一个路径，需要迭代一个个路径找下去，可能要把过程放到bfs里面，这个不行继续bfs遍历去找
    // 版本1 直接输出所有路径然后去遍历，直到没有路径为止

    std::vector<int> path = part_bfs->findPath(source_partition, target_partition);
    if (path.empty())
        return false;
    auto edges = partition_manager_.get_partition_adjacency(path[0], path[1]);
    return dfs_partition_search(source, edges.original_edges, path, target);
}

bool CompressedSearch::query_across_partitions_with_all_paths(int source, int target)
{
    int source_partition = g.get_partition_id(source);
    int target_partition = g.get_partition_id(target);
    bool flag = false;
    // 枚举所有路径
    std::vector<std::vector<int>> all_paths;
    std::vector<int> path;
    std::unordered_set<int> visited;
    bool result = dfs_paths_search(source_partition, target_partition, path, all_paths, visited, source, target);
    return result;
}

bool CompressedSearch::dfs_paths_search(int current_partition, int target_partition,
                                        std::vector<int> &path,
                                        std::vector<std::vector<int>> &all_paths,
                                        std::unordered_set<int> &visited,
                                        int source,
                                        int target)
{
    visited.insert(current_partition);
    path.push_back(current_partition);
    bool result = false;
    if (current_partition == target_partition)
    {
        all_paths.push_back(path);
        auto edges = partition_manager_.get_partition_adjacency(path[0], path[1]);
        result = dfs_partition_search(source, edges.original_edges, path, target);
        if (result)
            return true;
        cout << getCurrentTimestamp() << " 分区可达，但顶点之间没有可达路径" << endl;
    }
    else
    {
        // 获取当前分区的所有相邻分区
        std::vector<int> neighbors = partition_manager_.part_g.vertices[current_partition].LOUT;
        for (int neighbor : neighbors)
        {
            if (visited.find(neighbor) == visited.end())
            {
                result = dfs_paths_search(neighbor, target_partition, path, all_paths, visited, source, target);
                if (result)
                    return true;
            }
        }
    }

    path.pop_back();
    // 当前的分区改成未访问，在以此为根的生成树中已经遍历完成，不需要防止重复遍历
    // 从别的分区还可以有边过来，也就是说从别的点为根的子树中可以被访问，等下次访问到再说防止重复遍历的事情
    visited.erase(current_partition);
    return result;
}

/**
 * @brief 辅助方法，执行分区间的迭代式DFS搜索。
 * @param current_partition 当前分区ID。
 * @param target_partition 目标分区ID。
 * @param visited 已访问的分区集合。
 * @param path 从当前节点出发的可达路径集合
 * @return 如果可达返回 true，否则返回 false。
 */
bool CompressedSearch::dfs_partition_search(int u, std::vector<std::pair<int, int>> edges, std::vector<int> path, int target)
{
    auto current_partition = path[0];
    if (current_partition != g.get_partition_id(u))
    {
        throw std::logic_error("Current partition from path[] does not match the partition of node u");
    }
    bool result = false;
    for (auto &edge : edges)
    {
        cout << getCurrentTimestamp() << "当前边: " << edge.first << " -> " << edge.second << endl;
        // 上一个前序点无法到达这条边就下一轮循环
        if (is_index)
        {
            if (!query_index_within_partition(u, edge.first, current_partition))
                continue;
        }
        else
        {
            if (!query_within_partition(u, edge.first))
                continue;
        }
        // 如果下一个分区是终点分区就使用BFS查找，如果不是指向终点分区就继续递归找
        auto next_partition = path[1];
        if (g.get_partition_id(target) == next_partition)
        {
            if (is_index)
            {
                result = query_index_within_partition(edge.second, target, next_partition);
            }
            else
                result = query_within_partition(edge.second, target);
            if (result)
                return true;
        }
        else
        {
            std::vector<int> remain_path(path.begin() + 1, path.end());
            auto next_edges = partition_manager_.get_partition_adjacency(remain_path[0], remain_path[1]);
            result = dfs_partition_search(edge.second, next_edges.original_edges, remain_path, target);
            if (result)
                return true;
        }
    }

    return result;
}
// TODO:构建索引的时候用全局搜索，避免两个点绕过一个分区来相连
// 但是如果分区方法用连通度来计算，会不会有情况是加进去的点都是相连的呢
void CompressedSearch::build_partition_index(float ratio, size_t num_vertices)
{
#ifdef DEBUG
    std::cout << "Building partition index..." << std::endl;
#endif

    // 打印 mapping 信息
    std::cout << "Partition to Node Mapping:" << std::endl;
    for (const auto &partition_pair : partition_manager_.mapping)
    {
        if (partition_pair.second.size() == 1)
            continue;
        std::cout << "Partition " << partition_pair.first << ": ";
        for (const auto &node : partition_pair.second)
        {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    }
    this->ratio = ratio;
    this->num_vertices = num_vertices;
    for (auto &subgraph : this->partition_manager_.partition_subgraphs)
    {
        if (subgraph.first == -1)
            continue;
        // 计算分区的比例信息
        float total_ratio = compute_reach_ratio(subgraph.second);
        auto partition_id = subgraph.first;

        if (subgraph.second.get_num_vertices() == 1)
        {
            // 一个点就不管了
            continue;
        }
        // 根据情况构建索引,如果点数小于100那么就要建立一个子图的邻接矩阵
        // 如果g.get_num_vertices点数大于传进来的num_vertices,看分区图的ratio
        // ratio < 传进来的ratio,那么就建立PLL索引
        // ratio >= 传进来的ratio,那么就建立不可达的邻接表,对子图做V*V的循环,每个里面调用一次双边BFS,如果不可达就记录下来
        // 建立索引的方案是,在CompressSearch中建立三个数据结构
        if (subgraph.second.get_num_vertices() < num_vertices)
        {
            // 构建可达索引:可达矩阵
            // 映射后的号
            size_t index = 0;
            // 第一位是源节点，第二位是索引中的序号
            std::unordered_map<size_t, size_t> node_to_index;
            for (size_t i = 0; i < subgraph.second.vertices.size(); i++)
            {
                if (subgraph.second.vertices[i].in_degree > 0 || subgraph.second.vertices[i].out_degree > 0)
                {
                    node_to_index[i] = index++;
                }
            }

            small_mapping_[subgraph.first] = node_to_index;
            small_index_[subgraph.first] = std::vector<std::vector<std::bitset<1>>>(index, std::vector<std::bitset<1>>(index));

            // 初始化为全 0（实际上已经是全 0，不需要额外操作）
            for (size_t u = 0; u < subgraph.second.vertices.size(); u++)
            {
                if (subgraph.second.vertices[u].in_degree == 0 && subgraph.second.vertices[u].out_degree == 0)
                    continue;
                for (size_t v = 0; v < subgraph.second.vertices.size(); v++)
                {
                    if (subgraph.second.vertices[v].in_degree == 0 && subgraph.second.vertices[v].out_degree == 0)
                        continue;
                    if (u == v)
                        continue;
                    if (this->query_within_partition(u, v))
                    {
                        int mapped_u = node_to_index[u];
                        int mapped_v = node_to_index[v];

                        small_index_[subgraph.first][mapped_u][mapped_v].set(0); // 设置为 1 表示可达
                    }
                }
            }
            // 打印二维数组
            // std::cout<<"small_index_:"<<std::endl;
            // for (size_t i = 0; i < small_index_[subgraph.first].size(); i++) {
            //     for (size_t j = 0; j < small_index_[subgraph.first][i].size(); j++) {
            //         std::cout << small_index_[subgraph.first][i][j] << " ";
            //     }
            //     std::cout << std::endl;
            // }
        }
        else if (total_ratio < ratio)
        {
            // 构建pll
            pll_index_[subgraph.first] = new PLL(subgraph.second);
            pll_index_[subgraph.first]->offline_industry();
            // 打印 PLL 的 IN 和 OUT 集合

            std::cout << "PLL IN and OUT sets for partition " << subgraph.first << ":" << std::endl;
            for (size_t i = 0; i < pll_index_[subgraph.first]->IN.size(); ++i)
            {
                if (pll_index_[subgraph.first]->IN[i].size() > 0)
                {
                    std::cout << "Node " << i << " IN: ";
                    for (int in_node : pll_index_[subgraph.first]->IN[i])
                    {
                        std::cout << in_node << " ";
                    }
                    std::cout << std::endl;
                }
            }

            for (size_t i = 0; i < pll_index_[subgraph.first]->OUT.size(); ++i)
            {
                if (pll_index_[subgraph.first]->OUT[i].empty())
                    continue;
                std::cout << "Node " << i << " OUT: ";
                for (int out_node : pll_index_[subgraph.first]->OUT[i])
                {
                    std::cout << out_node << " ";
                }
                std::cout << std::endl;
            }
        }
        else
        {
            // 构建不可达的邻接表
            // 初始化映射和不可达邻接表
            std::unordered_map<size_t, size_t> &node_to_index = unreachable_mapping_[partition_id];
            std::vector<std::vector<size_t>> &unreachable_adj = unreachable_index_[partition_id];
            size_t current_index = 0;

            // 遍历子图中的所有节点对，记录不可达的点对
            for (size_t u = 0; u < subgraph.second.vertices.size(); u++)
            {
                if (subgraph.second.vertices[u].in_degree == 0 && subgraph.second.vertices[u].out_degree == 0)
                    continue;
                for (size_t v = 0; v < subgraph.second.vertices.size(); v++)
                {
                    if (subgraph.second.vertices[v].in_degree == 0 && subgraph.second.vertices[v].out_degree == 0)
                        continue;
                    if (u == v)
                        continue;
                    bool result = this->query_within_partition(u, v);
                    if (!result)
                    {

                        // 只在遇到不可达点对时进行映射
                        // 如果source未映射，则映射并扩展不可达邻接表
                        if (node_to_index.find(u) == node_to_index.end())
                        {
                            node_to_index[u] = current_index++;
                            unreachable_adj.emplace_back(); // 添加一个新的邻接列表
                        }

                        // 如果target未映射，则映射
                        if (node_to_index.find(v) == node_to_index.end())
                        {
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
            std::cout << "Unreachable Adjacency List " << std::endl;
            for (size_t i = 0; i < unreachable_adj.size(); i++)
            {
                std::cout << "Node " << i << ": ";
                for (size_t j = 0; j < unreachable_adj[i].size(); j++)
                {
                    std::cout << unreachable_adj[i][j] << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}
/**
 * @brief 遍历所有分区并打印其索引以及ratio
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> CompressedSearch::get_index_info()
{
    std::vector<std::string> lines;
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    lines.push_back("PRINTING INDEX INFO...........");
    // 打印图 g 的 ratio 值
    lines.push_back("Ratio for whole graph : " + std::to_string(g.get_ratio()));
    // 打印 mapping 信息
    lines.push_back("Partition to Node Mapping:");
    for (const auto &partition_pair : partition_manager_.mapping)
    {
        std::stringstream ss;
        ss << "Partition " << partition_pair.first << ": ";
        for (const auto &node : partition_pair.second)
        {
            ss << node << " ";
        }
        lines.push_back(ss.str());
    }

    for (const auto &subgraph : partition_manager_.partition_subgraphs)
    {
        if (subgraph.first == -1)
            continue;
        auto partition_id = subgraph.first;

        // 打印子图的 ratio 值
        lines.push_back("Ratio for partition " + std::to_string(partition_id) + ": " + std::to_string(subgraph.second.get_ratio()));

        if (subgraph.second.get_num_vertices() < num_vertices)
        {
            // 打印 small_index_
            lines.push_back("Small Index for partition " + std::to_string(partition_id) + ":");
            for (size_t i = 0; i < small_index_[partition_id].size(); i++)
            {
                std::stringstream ss;
                for (size_t j = 0; j < small_index_[partition_id][i].size(); j++)
                {
                    ss << small_index_[partition_id][i][j] << " ";
                }
                lines.push_back(ss.str());
            }
        }
        else if (subgraph.second.get_ratio() < ratio)
        {
            // 打印 PLL 的 IN 和 OUT 集合
            lines.push_back("PLL IN and OUT sets for partition " + std::to_string(partition_id) + ":");
            for (size_t i = 0; i < pll_index_[partition_id]->IN.size(); ++i)
            {
                if (pll_index_[partition_id]->IN[i].size() > 0)
                {
                    std::stringstream ss;
                    ss << "Node " << i << " IN: ";
                    for (int in_node : pll_index_[partition_id]->IN[i])
                    {
                        ss << in_node << " ";
                    }
                    lines.push_back(ss.str());
                }
            }

            for (size_t i = 0; i < pll_index_[partition_id]->OUT.size(); ++i)
            {
                if (pll_index_[partition_id]->OUT[i].empty())
                    continue;
                std::stringstream ss;
                ss << "Node " << i << " OUT: ";
                for (int out_node : pll_index_[partition_id]->OUT[i])
                {
                    ss << out_node << " ";
                }
                lines.push_back(ss.str());
            }
        }
        else
        {
            // 打印不可达邻接表
            lines.push_back("Unreachable Adjacency List for partition " + std::to_string(partition_id) + ":");
            const auto &unreachable_adj = unreachable_index_[partition_id];
            for (size_t i = 0; i < unreachable_adj.size(); i++)
            {
                std::stringstream ss;
                ss << "Node " << i << ": ";
                for (size_t j = 0; j < unreachable_adj[i].size(); j++)
                {
                    ss << unreachable_adj[i][j] << " ";
                }
                lines.push_back(ss.str());
            }
        }
    }

    return lines;
}