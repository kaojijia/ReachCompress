#include "PartitionManager.h"
#include "ReachRatio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>

/**
 * @brief 设置节点的分区ID。
 * @param node 节点编号。
 * @param partitionId 分区ID。
 */
PartitionManager::PartitionManager(Graph &graph) : g(graph), part_g(false)
{
    this->csr = new CSRGraph();
    this->csr->fromGraph(g);
}

// 建立分区图
void PartitionManager::build_partition_graph()
{
    // 清空之前的分区图
    part_g = Graph(false); // 假设分区图不需要存储边集

    // 清空分区之间的连接
    partition_adjacency.clear();

    // 清空分区和点的映射关系
    mapping.clear();

    // 清空分区子图
    partition_subgraphs.clear();
    partition_subgraphs_csr.clear();

    // 填充 mapping，记录每个分区包含的节点
    for (size_t node = 0; node < g.vertices.size(); ++node)
    {
        int partition_id = g.get_partition_id(node);
        mapping[partition_id].insert(node);
    }

    // 更新分区之间的连接
    update_partition_connections();

    // 使用一个临时的映射来累积边的权重
    std::unordered_map<int, std::unordered_map<int, int>> temp_edges;

    // 遍历分区之间的连接，累积边的数量
    for (const auto &source_pair : partition_adjacency)
    {
        int source_partition = source_pair.first;
        const auto &targets = source_pair.second;
        for (const auto &target_pair : targets)
        {
            int target_partition = target_pair.first;
            const auto &edge_info = target_pair.second;
            temp_edges[source_partition][target_partition] += edge_info.edge_count;
        }
    }

    // 添加累积后的边到分区图，但是相同的边只存一条
    for (const auto &source_pair : temp_edges)
    {
        int source_partition = source_pair.first;
        const auto &targets = source_pair.second;
        for (const auto &target_pair : targets)
        {
            int target_partition = target_pair.first;
            part_g.addEdge(source_partition, target_partition, true);
        }
    }

    // 分区图的每个点分区应该一样，不然没法找
    for(auto& node : part_g.vertices){
        if(node.in_degree == 0 && node.out_degree == 0){
            node.partition_id = -1;
        }
        node.partition_id = 1;
    }
    part_g.set_max_node_id(part_g.vertices.size());
    this->part_csr = new CSRGraph();
    part_csr->fromGraph(part_g);

    // 为每个分区创建一个子图对象
    for (const auto &partition_pair : mapping)
    {
        int partition_id = partition_pair.first;
        partition_subgraphs[partition_id] = Graph(false); // 子图不需要存储边集
    }

    // 遍历原始图的所有节点，构建分区子图
    for (size_t u = 0; u < g.vertices.size(); ++u)
    {
        int u_partition = g.get_partition_id(u);

        // 获取对应的子图
        Graph &subgraph = partition_subgraphs[u_partition];

        // 确保子图中的顶点列表足够大
        if (u >= subgraph.vertices.size())
        {
            subgraph.vertices.resize(u + 1);
        }

        // 复制节点分区
        subgraph.vertices[u].partition_id = u_partition;

        // 遍历出边
        for (int v : g.vertices[u].LOUT)
        {
            int v_partition = g.get_partition_id(v);
            if (v_partition == u_partition)
            {
                // 边在同一分区内，添加到子图中
                if (v >= subgraph.vertices.size())
                {
                    subgraph.vertices.resize(v + 1);
                }

                // 添加边到子图
                subgraph.addEdge(u, v);
            }
        }
    }

    // 为每个子图构建CSR图
    for (auto &partition_pair : partition_subgraphs)
    {
        Graph &subgraph = partition_pair.second;
        auto csr = std::make_shared<CSRGraph>();
        csr->fromGraph(subgraph);
        partition_subgraphs_csr[partition_pair.first] = csr;
    }


    // 打印分区图信息（可选）
    // std::cout << "Partition graph constructed with " << temp_edges.size() << " partitions." << std::endl;
}

// void PartitionManager::update_partition_info(int node, uint16_t old_partition_id, uint16_t new_partition_id){

// }


void PartitionManager::update_partition_connections()
{
    for (size_t u = 0; u < g.vertices.size(); ++u)
    {
        if (g.vertices[u].LOUT.empty() && g.vertices[u].LIN.empty()) continue; // Skip nodes with no edges
        int u_partition = g.get_partition_id(u);
        if (u_partition == -1) continue;
        
        for (const auto &v : g.vertices[u].LOUT)
        {
            int v_partition = g.get_partition_id(v);
            if (u_partition != v_partition)
            {
                PartitionEdge &pe = partition_adjacency[u_partition][v_partition];
                if (std::find(pe.original_edges.begin(), pe.original_edges.end(), std::make_pair(static_cast<int>(u), v)) == pe.original_edges.end()) {
                    pe.original_edges.emplace_back(u, v);
                    pe.edge_count++;
                }
            }
        }
        for (const auto &v : g.vertices[u].LIN)
        {
            int v_partition = g.get_partition_id(v);
            if (u_partition != v_partition)
            {
                PartitionEdge &pe = partition_adjacency[v_partition][u_partition];
                if (std::find(pe.original_edges.begin(), pe.original_edges.end(), std::make_pair(v, static_cast<int>(u))) == pe.original_edges.end()) {
                    pe.original_edges.emplace_back(v, u);
                    pe.edge_count++;
                }
            }
        }
    }
}

void PartitionManager::update_partition_info(int node, int old_partition_id, int new_partition_id)
{
    // 更新节点的分区id
    if(g.vertices[node].partition_id != old_partition_id) 
        std::cout<<"error: old partition id not match"<<std::endl;
    g.vertices[node].partition_id = new_partition_id;

    if (g.vertices[node].LOUT.empty() && g.vertices[node].LIN.empty()) return; // Skip nodes with no edges
    if (node > g.get_num_vertices()) return;
    if (new_partition_id == -1 || old_partition_id == -1 || old_partition_id == new_partition_id) return;

    // 更新mapping
    mapping[old_partition_id].erase(node);
    mapping[new_partition_id].insert(node);
    if(mapping[old_partition_id].empty()){
        mapping.erase(old_partition_id);
    }

    // 更新分区图上的节点id
    // 移动节点失败，还原时用,因为原有节点删除的时候可能连带着把分区删了
    // 后面就算分区间加边可以加新的点，也没办法把新的点的分区加出来
    // 因此这里给补一下，如果分区不存在的话（加了mapping之后mapping的大小是1）那么这个分区的分区号设置一下
    // 分区图应该同属于一个分区1
    if(mapping[new_partition_id].size() == 1){
        part_g.vertices[node].partition_id = 1;
    }

    // 更新分区之间的连接和分区图
    for(auto v: g.vertices[node].LOUT)
    {
        // 删除原有的边和分区图上的信息,如果和前驱点相同的就不用管了
        if(g.vertices[v].partition_id != old_partition_id){
            // 更新分区之间的连接
            PartitionEdge &pe_old = partition_adjacency[old_partition_id][g.vertices[v].partition_id];
            for (auto it = pe_old.original_edges.begin(); it != pe_old.original_edges.end(); ++it)
            {
                if (*it == std::make_pair(node, v))
                {
                    pe_old.original_edges.erase(it);
                    break;
                }
            }
            pe_old.edge_count--;
            if(pe_old.edge_count == 0){
                partition_adjacency[old_partition_id].erase(g.vertices[v].partition_id);
            }
            if(partition_adjacency[old_partition_id].empty())
            {
                partition_adjacency.erase(old_partition_id);
            }
            // 删除分区图上的边
            part_g.removeEdge(old_partition_id, g.vertices[v].partition_id);
            // 清理孤立分区
        }

        //添加新的边，如果和后继点相同分区就不用了
        if(g.vertices[v].partition_id != new_partition_id){
            //分区之间的连接加条边。这里原本没有pe的话会新创建一个
            PartitionEdge &pe = partition_adjacency[new_partition_id][g.vertices[v].partition_id];
            pe.original_edges.emplace_back(node, v);
            pe.edge_count++;
            //分区图上加边
            part_g.addEdge(new_partition_id, g.vertices[v].partition_id);
        }
    }


    // 对于顶点的入边，减掉原有分区的，新分区的入边加一个
    for(auto v: g.vertices[node].LIN)
    {
        if(g.vertices[v].partition_id != old_partition_id){
            // 删除原有的边
            PartitionEdge &pe_old = partition_adjacency[g.vertices[v].partition_id][old_partition_id];
            for (auto it = pe_old.original_edges.begin(); it != pe_old.original_edges.end(); ++it)
            {
                if (*it == std::make_pair(v, node))
                {
                    pe_old.original_edges.erase(it);
                    break;
                }
            }
            pe_old.edge_count--;
            if(pe_old.edge_count == 0){
                partition_adjacency[g.vertices[v].partition_id].erase(old_partition_id);
            }
            if(partition_adjacency[g.vertices[v].partition_id].empty())
            {
                partition_adjacency.erase(g.vertices[v].partition_id);
            }
            // 删除分区图上的边
            part_g.removeEdge(g.vertices[v].partition_id, old_partition_id);
        }

        //添加新的边，如果和前驱点相同分区就不用了
        if(g.vertices[v].partition_id != new_partition_id){
            PartitionEdge &pe = partition_adjacency[g.vertices[v].partition_id][new_partition_id];
            pe.original_edges.emplace_back(v, node);
            pe.edge_count++;
            //分区图上加边
            part_g.addEdge(g.vertices[v].partition_id, new_partition_id);
        } 
    }

    // 清理孤立分区
    if(partition_adjacency[old_partition_id].empty())
    {
        partition_adjacency.erase(old_partition_id);
    }
}

void PartitionManager::print_equivalence_mapping() const
{
    if (equivalence_mapping == nullptr)
    {
        std::cerr << "Equivalence mapping is not initialized." << std::endl;
        return;
    }

    for (size_t i = 0; i < g.vertices.size(); ++i)
    {
        std::cout << "Node " << i << ": ";
        if (equivalence_mapping[i] != nullptr)
        {
            std::cout << equivalence_mapping[i][0]; // Assuming each node has at least one equivalence
        }
        else
        {
            std::cout << "No equivalence";
        }
        std::cout << std::endl;
    }
}
void PartitionManager::read_equivalance_info(const std::string &filename)
{
    std::ifstream mapping_file(filename);
    if (!mapping_file.is_open())
    {
        std::cerr << "Failed to open mapping file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::vector<std::vector<uint32_t>> temp_mapping;

    while (std::getline(mapping_file, line))
    {
        std::istringstream iss(line);
        int node;
        uint32_t equivalance;
        if (!(iss >> node >> equivalance))
        {
            std::cerr << "Invalid line format in equivalence file: " << line << std::endl;
            continue;
        }

        // 如果大于了要做扩展，因为g读取的文件是强连通分量压缩后的文件，所以可能会有节点记录不全
        // mapping中的是全的，可以拿来对g做补全
        if (node >= g.vertices.size())
        {
            int size = g.vertices.size();
            g.vertices.resize(node + 1);
            for (int i = size; i <= node; ++i)
            {
                g.vertices[i].partition_id = -1;
                g.vertices[i].LIN.clear();
                g.vertices[i].LOUT.clear();
                g.vertices[i].in_degree = 0;
                g.vertices[i].out_degree = 0;
                g.vertices[i].equivalance = -1;
            }
        }

        // 更新 Graph 中的 vertices
        g.vertices[node].equivalance = equivalance;

        // 添加到临时映射
        if (node >= temp_mapping.size())
        {
            temp_mapping.resize(node + 1);
        }
        temp_mapping[node].push_back(equivalance);
    }

    mapping_file.close();

    // 将临时映射转换为二维 uint32_t 数组
    equivalence_mapping = new uint32_t *[temp_mapping.size()];
    for (size_t i = 0; i < temp_mapping.size(); ++i)
    {
        equivalence_mapping[i] = new uint32_t[temp_mapping[i].size()];
        for (size_t j = 0; j < temp_mapping[i].size(); ++j)
        {
            equivalence_mapping[i][j] = temp_mapping[i][j];
        }
    }

    std::cout << "Mapping completed using file: " << filename << std::endl;
}

std::unordered_map<int, PartitionEdge> PartitionManager::get_partition_adjacency(int partitionId)
{
    auto it_outer = partition_adjacency.find(partitionId);
    if (it_outer == partition_adjacency.end())
    {
        //throw std::out_of_range("Partition not found.");
        return {};
    }
    return partition_adjacency.at(partitionId);
}

// 获取两个分区之间的连接
PartitionEdge PartitionManager::get_partition_adjacency(int u, int v)
{
    auto it_outer = partition_adjacency.find(u);
    if (it_outer == partition_adjacency.end())
    {
        throw std::out_of_range("Partition U not found.");
    }

    const auto &inner_map = it_outer->second;
    auto it_inner = inner_map.find(v);
    if (it_inner == inner_map.end())
    {
        throw std::out_of_range("Partition V not found.");
    }

    return it_inner->second;
}