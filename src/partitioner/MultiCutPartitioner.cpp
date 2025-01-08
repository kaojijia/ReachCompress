#include "partitioner/MultiCutPartitioner.h"
#include "CSR.h"
#include "graph.h"
#include "PartitionManager.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>

int min_cut = INT_MAX;
int max_partition_difference = 50000;// 分出来的两个区最多差多少个顶点
int min_size = 20; // 最小分区大小
int total_iterations = 50000; // 随机化试验次数
int max_cut_edges = 20; // 最大割边数
int max_partitions_count = 6; // 最大分区数

// 辅助函数：划分弱连通分量，先做初步分区
void initial_partition(CSRGraph &csr, std::vector<int> &partition)
{
    int num_nodes = csr.max_node_id + 1;
    for (int i = 0; i < num_nodes; ++i)
    {
        if (csr.getInDegree(i) == 0 && csr.getOutDegree(i) == 0)
        {
            continue; // 忽略孤立节点
        }
        partition[i] = i; // 初始分区为节点自身
    }

    // 使用 DFS 遍历弱连通分量
    std::vector<bool> visited(num_nodes, false);
    for (int i = 0; i < num_nodes; ++i)
    {
        if (visited[i] || partition[i] != i)
        {
            continue;
        }

        std::vector<int> component;
        std::vector<int> stack;
        stack.push_back(i);

        while (!stack.empty())
        {
            int node = stack.back();
            stack.pop_back();

            if (visited[node])
            {
                continue;
            }

            visited[node] = true;
            component.push_back(node);

            uint32_t out_degree = 0;
            auto *out_neighbors = csr.getOutgoingEdges(node, out_degree);
            for (int j = 0; j < out_degree; ++j)
            {
                if (!visited[out_neighbors[j]] && partition[out_neighbors[j]] == out_neighbors[j])
                {
                    stack.push_back(out_neighbors[j]);
                }
            }
            uint32_t in_degree = 0;
            auto *in_neighbors = csr.getIncomingEdges(node, in_degree);
            for (int j = 0; j < in_degree; ++j)
            {
                if (!visited[in_neighbors[j]] && partition[in_neighbors[j]] == in_neighbors[j])
                {
                    stack.push_back(in_neighbors[j]);
                }
            }
        }

        // 为每个弱连通分量分配分区ID是DFS的根节点
        for (int node : component)
        {
            partition[node] = i;
        }
    }
}

// 辅助函数：并查集查找
int find(std::vector<int> &parent, int i)
{
    if (parent[i] != i)
    {
        parent[i] = find(parent, parent[i]);
    }
    return parent[i];
}

// 辅助函数：并查集合并
void Union(std::vector<int> &parent, std::vector<int> &rank, int x, int y)
{
    int xroot = find(parent, x);
    int yroot = find(parent, y);

    if (rank[xroot] < rank[yroot])
    {
        parent[xroot] = yroot;
    }
    else if (rank[xroot] > rank[yroot])
    {
        parent[yroot] = xroot;
    }
    else
    {
        parent[yroot] = xroot;
        rank[xroot]++;
    }
}

// Karger实现
pair<int, int> kargerMinCutCSR(CSRGraph &csr, std::vector<int> &partition, int partition_id)
{

    std::vector<std::pair<int, int>> edges;
    for (int u = 0; u < csr.max_node_id + 1; ++u)
    {
        if (partition[u] != partition_id)
        {
            continue; 
        }

        uint32_t degree;
        uint32_t *neighbors = csr.getOutgoingEdges(u, degree);
        for (int i = 0; i < degree; ++i)
        {
            if (partition[neighbors[i]] == partition_id)
            {
                edges.emplace_back(u, neighbors[i]);
            }
        }
    }

    if (edges.empty())
    {
        return {0, 0};
    }

    vector<int> nodes;
    std::vector<int> parent(csr.max_node_id + 1, -1);
    std::vector<int> rank(csr.max_node_id + 1, -1);
    int V = 0;
    for (int i = 0; i < csr.max_node_id + 1; ++i)
    {
        if (partition[i] == partition_id)
        {
            V++; // 统计有效节点数量
            parent[i] = i;
            nodes.push_back(i);
        }
        else
        {
            parent[i] = -1;
            rank[i] = -1;
        }
    }

    //随机收缩
    std::vector<bool> edge_removed(edges.size(), false); // 标记边是否已移除
    int remaining_nodes = V;                             // 当前剩余的有效节点数
    int removed_edges = 0;                               // 已移除的边数量

    while (remaining_nodes > 2 && removed_edges < edges.size())
    {
        // 随机选取一条边
        int random_index = rand() % edges.size();
        while (edge_removed[random_index])
        {
            random_index = rand() % edges.size();
        }

        int subset1 = find(parent, edges[random_index].first);
        int subset2 = find(parent, edges[random_index].second);

        // 如果边的两个端点在同一个集合中，跳过
        if (subset1 == subset2)
        {
            edge_removed[random_index] = true;
            removed_edges++;
            continue;
        }

        // 合并两个集合
        remaining_nodes--;
        Union(parent, rank, subset1, subset2);
        edge_removed[random_index] = true; // 标记为已移除
        removed_edges++;
    }

    // Step 4: 统计割边数
    int cutedges = 0;
    for (size_t i = 0; i < edges.size(); ++i)
    {
        if (edge_removed[i])
            continue;
        int subset1 = find(parent, edges[i].first);
        int subset2 = find(parent, edges[i].second);
        if (subset1 != subset2)
            cutedges++;
    }

    // Step 5: 更新节点的分区
    for (int i = 0; i < csr.max_node_id + 1; ++i)
    {
        if (partition[i] == partition_id)
        {
            partition[i] = find(parent, i); // 分区设置为最终的父节点
        }
    }

    // TODO: 返回两个分区点数的差异
    int partition_difference;
    std::unordered_map<int, int> partition_count;
    for (auto i : nodes)
    {
        if (partition[i] == -1)
            continue;
        partition_count[partition[i]]++;
    }

    if (partition_count.size() != 2)
    {
        partition_difference = -1; // 如果分区数不等于2，返回-1表示错误
    }

    auto it = partition_count.begin();
    int count1 = it->second;
    int count2 = (++it)->second;

    int flag = 1;
    partition_difference = std::abs(count1 - count2);
    if(partition_difference > max_partition_difference)
    {
        flag = 0; // 如果分区差异大于阈值，返回0表示错误
    }
    if(count1 < min_size || count2 < min_size)
    {
        flag = 0; // 如果任一分区小于最小大小，返回0表示错误
    }

    return {cutedges, flag};
}

// MultiCutPartitioner::partition 的实现
void MultiCutPartitioner::partition(Graph &g, PartitionManager &partition_manager)
{
    srand(time(0)); // 设置随机种子

    CSRGraph csr;
    csr.fromGraph(g); // 从图中构建CSR图

    std::shared_ptr<std::vector<int>> best_partition = std::make_shared<std::vector<int>>(csr.max_node_id + 1, -1);

    initial_partition(csr, *best_partition); // 初始划分弱连通分量

    map<int, int> partition_statics;
    for (int i = 0; i < best_partition->size(); i++)
    {
        if ((*best_partition)[i] != -1)
        {
            partition_statics[(*best_partition)[i]]++;
        }
    }
    int max_size = 0;
    int max_partition = 0;
    for (auto const &[key, value] : partition_statics)
    {
        if (value > max_size)
        {
            max_partition = key;
            max_size = value;
        }
    }
    int new_max_size = max_partitions_count+partition_statics.size(); 

    int times = 0;
    while (partition_statics.size() < new_max_size && max_size > min_size)
    {
        if(times++ > 50)
            break; 
        int current_min_cut = min_cut;
        auto temp_partition = std::make_shared<std::vector<int>>(csr.max_node_id + 1, -1);
        for (int i = 0; i <= csr.max_node_id; ++i)
        {
            (*temp_partition)[i] = (*best_partition)[i];
        }
        // 进行多次随机化试验
        for (int i = 0; i < total_iterations; ++i)
        {
            auto partition = std::make_shared<std::vector<int>>(csr.max_node_id + 1, -1); // 初始化智能指针
            for (int i = 0; i <= csr.max_node_id; ++i)
            {
                (*partition)[i] = (*best_partition)[i];
            }
            auto result = kargerMinCutCSR(csr, *partition, max_partition); // 计算最小割
            int cut = result.first;
            int flag = result.second;
            if (cut < current_min_cut && flag)
            {
                current_min_cut = cut;
                temp_partition = partition; // 直接将指针赋值
            }
        }
        if(current_min_cut > max_cut_edges && current_min_cut != min_cut)
            break; // 如果最小割大于预设值，停止迭代
        // 更新最佳分区
        best_partition = temp_partition;

        // 统计
        partition_statics.clear();
        for (int i = 0; i < best_partition->size(); i++)
        {
            if ((*best_partition)[i] != -1)
            {
                partition_statics[(*best_partition)[i]]++;
            }
        }
        max_size = 0;
        for (auto const &[key, value] : partition_statics)
        {
            if (value > max_size)
            {
                max_size = value;
                max_partition = key;
            }
        }
    }

    // std::cout << "The minimum cut is " << min_cut << std::endl;

    unordered_map<int, set<int>> partition_mapping;
    // 输出每个节点的分区
    for (size_t i = 0; i < best_partition->size(); ++i)
    {
        if ((*best_partition)[i] == -1)
        {
            continue;
        }
        else
        {
            partition_mapping[(*best_partition)[i]].insert(i);
            std::cout << "Node " << i << " is in partition " << (*best_partition)[i] << std::endl;
        }
    }

    // 将分区信息存入到 graph.vertices[i].partition_id 中
    for (size_t i = 0; i < best_partition->size(); ++i)
    {
        if ((*best_partition)[i] != -1)
        {
            g.vertices[i].partition_id = (*best_partition)[i];
        }
    }
    // 建立分区图和对应的信息
    partition_manager.update_partition_connections();
    partition_manager.build_partition_graph();

    // //构造顶点的分区可达集合
    // auto i = get_reachable_partitions(g, 3);
    // for(int j = 0; j < g.vertices.size(); j++){
    //     if(g.vertices[j].partition_id == -1) continue;
    //     cout<< "Node " << j << " has reachable partition :"<<endl;
    //     cout << "IN: ";
    //     for(auto const& [key, value] : i[j]){
    //         if(value.in) cout << key << " ";
    //     }
    //     cout << endl;
    //     cout<< "OUT: ";
    //     for(auto const& [key, value] : i[j]){
    //         if(value.out) cout << key << " ";
    //     }
    //     cout<<endl;
    // }
}