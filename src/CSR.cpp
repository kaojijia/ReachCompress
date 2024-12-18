//CSR.cpp

#include "CSR.h"


// 从边集文件中读取图数据并构建 CSR 结构
bool CSRGraph::fromFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }

    uint32_t u, v;
    uint32_t max_node = 0;
    std::vector<std::pair<uint32_t, uint32_t>> edges;

    // 第一遍遍历：收集所有边并找到最大节点 ID
    while (infile >> u >> v) {
        edges.emplace_back(u, v);
        if (u > max_node) max_node = u;
        if (v > max_node) max_node = v;
    }

    infile.close();

    
    max_node_id = max_node;
    num_edges = edges.size();

    // 分配内存
    out_row_pointers = new uint32_t[max_node_id + 2];
    in_row_pointers = new uint32_t[max_node_id + 2];
    partitions = new int16_t[max_node_id];
    std::memset(out_row_pointers, 0, (max_node_id + 2) * sizeof(uint32_t));
    std::memset(in_row_pointers, 0, (max_node_id + 2) * sizeof(uint32_t));
    for (uint32_t i = 0; i < max_node_id; ++i) {
        partitions[i] = -1;
    }
    out_column_indices = new uint32_t[num_edges];
    in_column_indices = new uint32_t[num_edges];

    // 统计出边和入边的数量
    std::vector<uint32_t> out_counts(max_node_id, 0);
    std::vector<uint32_t> in_counts(max_node_id, 0);
    for (const auto& edge : edges) {
        out_counts[edge.first]++;
        in_counts[edge.second]++;
    }

    // 构建 row_pointers
    out_row_pointers[0] = 0;
    in_row_pointers[0] = 0;
    for (uint32_t i = 0; i < max_node_id; ++i) {
        out_row_pointers[i + 1] = out_row_pointers[i] + out_counts[i];
        in_row_pointers[i + 1] = in_row_pointers[i] + in_counts[i];
    }

    // 填充 column_indices
    std::vector<uint32_t> out_offsets(max_node_id, 0);
    std::vector<uint32_t> in_offsets(max_node_id, 0);
    for (const auto& edge : edges) {
        uint32_t src = edge.first;
        uint32_t dst = edge.second;
        out_column_indices[out_row_pointers[src] + out_offsets[src]] = dst;
        in_column_indices[in_row_pointers[dst] + in_offsets[dst]] = src;
        out_offsets[src]++;
        in_offsets[dst]++;
    }

    // 对每个节点的出边和入边进行排序
    for (uint32_t i = 0; i < max_node_id; ++i) {
        uint32_t out_start = out_row_pointers[i];
        uint32_t out_end = out_row_pointers[i + 1];
        std::sort(out_column_indices + out_start, out_column_indices + out_end);

        uint32_t in_start = in_row_pointers[i];
        uint32_t in_end = in_row_pointers[i + 1];
        std::sort(in_column_indices + in_start, in_column_indices + in_end);
    }

    //计算节点数量
    for (uint32_t i = 0; i < max_node_id; i++)
    {
        if(out_row_pointers[i]==out_row_pointers[i+1]&&in_row_pointers[i]==in_row_pointers[i+1])
            continue;
        num_nodes++;
    }
    


    return true;
}

// 从现有Graph类创建CSR
bool CSRGraph::fromGraph(const Graph& graph) {
    // 设置最大节点 ID
    max_node_id = graph.vertices.size() - 1;

    // 填充 column_indices的vector
    std::vector<uint32_t> out_offsets(max_node_id+1, 0);
    std::vector<uint32_t> in_offsets(max_node_id+1, 0);
    
    // 统计出边和入边的数量
    std::vector<uint32_t> out_counts(max_node_id+1, 0);
    std::vector<uint32_t> in_counts(max_node_id+1, 0);

    // CSR的顶点行
    out_row_pointers = new uint32_t[max_node_id + 2];
    in_row_pointers = new uint32_t[max_node_id + 2];
    partitions = new int16_t[max_node_id+1];

    // 统计总边数
    num_edges = graph.get_num_edges();

    out_column_indices = new uint32_t[num_edges];
    in_column_indices = new uint32_t[num_edges];


    std::memset(out_row_pointers, 0, (max_node_id + 2) * sizeof(uint32_t));
    std::memset(in_row_pointers, 0, (max_node_id + 2) * sizeof(uint32_t));
    for (uint32_t i = 0; i < max_node_id + 1; ++i) {
        partitions[i] = graph.vertices[i].partition_id;
    }



    for (size_t u = 0; u < graph.vertices.size(); u++) {
        if(graph.vertices[u].LOUT.size()==0)continue;
        for (size_t v_idx = 0; v_idx < graph.vertices[u].LOUT.size(); v_idx++) {
            uint32_t v = graph.vertices[u].LOUT[v_idx];
            out_counts[u]++;
            if (v < max_node_id+1) { // 确保 v 不越界
                in_counts[v]++;
            }
        }
    }

    // 构建 row_pointers
    out_row_pointers[0] = 0;
    in_row_pointers[0] = 0;
    for (uint32_t i = 0; i < max_node_id+1; ++i) {
        out_row_pointers[i + 1] = out_row_pointers[i] + out_counts[i];
        in_row_pointers[i + 1] = in_row_pointers[i] + in_counts[i];
    }


    for (size_t u = 0; u < graph.vertices.size(); u++) {
        if(graph.vertices[u].LOUT.size()==0)continue;
        // 对出边进行排序
        std::vector<uint32_t> sorted_LOUT(graph.vertices[u].LOUT.begin(), graph.vertices[u].LOUT.end());
        std::sort(sorted_LOUT.begin(), sorted_LOUT.end());

        for (const auto& v : sorted_LOUT) {
            if (v >= max_node_id+1) continue; // 确保 v 不越界

            // 添加出边
            out_column_indices[out_row_pointers[u] + out_offsets[u]] = v;
            out_offsets[u]++;

            // 添加入边
            in_column_indices[in_row_pointers[v] + in_offsets[v]] = u;
            in_offsets[v]++;
        }
    }

    // 对每个节点的入边进行排序
    for (uint32_t i = 0; i < max_node_id+1; ++i) {
        uint32_t in_start = in_row_pointers[i];
        uint32_t in_end = in_row_pointers[i + 1];
        std::sort(in_column_indices + in_start, in_column_indices + in_end);
    }

    // 获取总节点数
    // for(auto i:graph.vertices){
    //     if(i.in_degree==0&&i.out_degree==0)continue;
    //     num_nodes++;
    // }
    num_nodes = graph.get_num_vertices();
    //获取每个节点的分区号
    // this->partitions = new int16_t[max_node_id+1];
    // for(auto node:graph.vertices){
    //     this->partitions[node.partition_id] = node.partition_id;
    // }
    // if(graph.vertices[0].LOUT.size()==0 && graph.vertices[0].LIN.size()==0){
    //     removeNode(0);
    // }
    return true;
}



// 获取某个节点的所有出边
uint32_t* CSRGraph::getOutgoingEdges(uint32_t node, uint32_t& degree) const {
    if (node > max_node_id) {
        degree = 0;
        return nullptr;
    }
    degree = out_row_pointers[node + 1] - out_row_pointers[node];
    return out_column_indices + out_row_pointers[node];
}

// 获取某个节点的所有入边
uint32_t* CSRGraph::getIncomingEdges(uint32_t node, uint32_t& degree) const {
    if (node > max_node_id) {
        degree = 0;
        return nullptr;
    }
    degree = in_row_pointers[node + 1] - in_row_pointers[node];
    return in_column_indices + in_row_pointers[node];
}

// 获取某个节点的出度
uint32_t CSRGraph::getOutDegree(uint32_t node) const {
    if (node > max_node_id) return 0;
    return out_row_pointers[node + 1] - out_row_pointers[node];
}

// 获取某个节点的入度
uint32_t CSRGraph::getInDegree(uint32_t node) const {
    if (node > max_node_id) return 0;
    return in_row_pointers[node + 1] - in_row_pointers[node];
}

// 增加节点
bool CSRGraph::addNode() {
    uint32_t new_max_node_id = max_node_id + 1;
    num_nodes++;

    // 重新分配 row_pointers 和 partitions
    uint32_t* new_out_row_pointers = new uint32_t[new_max_node_id + 2];
    uint32_t* new_in_row_pointers = new uint32_t[new_max_node_id + 2];
    int16_t* new_partitions = new int16_t[new_max_node_id];

    // 复制旧数据
    std::memcpy(new_out_row_pointers, out_row_pointers, (max_node_id + 2) * sizeof(uint32_t));
    std::memcpy(new_in_row_pointers, in_row_pointers, (max_node_id + 2) * sizeof(uint32_t));
    std::memcpy(new_partitions, partitions, max_node_id * sizeof(int16_t));

    // 初始化新节点的 row_pointers 和 partition
    new_out_row_pointers[new_max_node_id] = new_out_row_pointers[max_node_id];
    new_in_row_pointers[new_max_node_id] = new_in_row_pointers[max_node_id];
    new_partitions[max_node_id] = -1;

    // 释放旧内存
    delete[] out_row_pointers;
    delete[] in_row_pointers;
    delete[] partitions;

    // 更新指针和数量
    out_row_pointers = new_out_row_pointers;
    in_row_pointers = new_in_row_pointers;
    partitions = new_partitions;
    max_node_id = new_max_node_id;

    return true;
}

// 删除节点

bool CSRGraph::removeNode(uint32_t node) {
    if (node > max_node_id) {
        std::cerr << "错误: 节点 " << node << " 超出范围." << std::endl;
        return false;
    }

    // 检查节点是否已经被删除（即没有任何出边和入边）
    if (out_row_pointers[node] == out_row_pointers[node + 1] &&
        in_row_pointers[node] == in_row_pointers[node + 1]) {
        std::cout << "节点 " << node << " 已经没有边，无需删除." << std::endl;
        return false;
    }


    // 收集顶点的所有出边 (node -> target)
    uint32_t out_start = out_row_pointers[node];
    uint32_t out_end = out_row_pointers[node + 1];
    uint32_t out_degree = out_end - out_start;

    // 删除out_column中的node的出边和node的入边，同时更新row_pointers
    uint32_t out_num_edges = num_edges; 
    for (uint32_t i = 0; i < max_node_id+1; ++i) {
        if(i == node){
            uint32_t* pos = &out_column_indices[out_row_pointers[i]];
            //out_num_edges：减去当前的之后还剩下多少条边
            std::memmove(pos, pos+out_degree, (out_column_indices + out_num_edges - pos - out_degree) * sizeof(uint32_t));
            out_num_edges -= out_degree;
            for (uint32_t j = i + 1; j < max_node_id+2; ++j) {
                out_row_pointers[j] -= out_degree;
            }
        }
        else{
            //非node节点，在出边里找是否有node，如果有就删除这个边
            uint32_t i_out_degree = out_row_pointers[i + 1] - out_row_pointers[i];
            uint32_t* out_ptr = out_column_indices + out_row_pointers[i];
            uint32_t* pos = std::find(out_ptr, out_ptr + i_out_degree, node);
            if (pos != out_ptr + i_out_degree) {
                std::memmove(pos, pos + 1, (out_column_indices + out_num_edges - pos - 1) * sizeof(uint32_t));
                out_num_edges--;
                for (uint32_t j = i + 1; j < max_node_id+2; ++j) {
                    out_row_pointers[j]--;
                }

            }

        }
    }

    // 收集顶点的所有入边 (source -> node)
    uint32_t in_degree = in_row_pointers[node + 1] - in_row_pointers[node];
    uint32_t in_num_edges = num_edges; 
    for (uint32_t i = 0; i < max_node_id+1; i++)
    {
        if (i==node)
        {
            uint32_t* pos = &in_column_indices[in_row_pointers[i]];
            //in_num_edges：减去当前的之后还剩下多少条边
            std::memmove(pos, pos+in_degree, (in_column_indices + in_num_edges - pos - in_degree) * sizeof(uint32_t));
            in_num_edges -= in_degree;
            for (uint32_t j = i + 1; j < max_node_id+2; ++j) {
                in_row_pointers[j] -= in_degree;
            }
        }else{
            //非node节点，在入边里找是否有node，如果有就删除这个边

            uint32_t i_in_start = in_row_pointers[i];
            uint32_t i_in_end = in_row_pointers[i + 1];
            uint32_t i_in_degree = in_row_pointers[i + 1] - in_row_pointers[i];
            uint32_t* in_ptr = in_column_indices + in_row_pointers[i];
            uint32_t* pos = std::find(in_ptr, in_ptr + i_in_degree, node);
            if (pos != in_ptr + i_in_degree) {
                printCSRs();
                std::memmove(pos, pos + 1, (in_column_indices + in_num_edges - pos - 1) * sizeof(uint32_t));
                in_num_edges--;
                for (uint32_t j = i + 1; j < max_node_id+2; ++j) {
                    in_row_pointers[j]--;
                }

            }
        }
        
    }
    
    num_nodes--;
    
    //如果删的是最大值的节点的话，数组要收缩一个单位
    if (node == max_node_id) {
        while (max_node_id > 0 &&
               out_row_pointers[max_node_id + 1] == out_row_pointers[max_node_id] &&
               in_row_pointers[max_node_id + 1] == in_row_pointers[max_node_id]) {
            max_node_id--;
        }
    }

    if(in_num_edges != out_num_edges){
        std::cout<<"逻辑错误"<<std::endl;
        return false;
    }

    num_edges = out_num_edges;
    out_row_pointers[max_node_id+1] = num_edges;
    printAllInfo();
    printCSRs();
    std::cout << "节点 " << node << " 删除完成." << std::endl;


    return true;

}
// 增加边（假定新点的id不会大于当前最大节点的id）
bool CSRGraph::addEdge(uint32_t u, uint32_t v) {
    
    if (u > max_node_id || v > max_node_id) return false;
    if(u==v)return false;

    // 更新顶点数目
    if(out_row_pointers[u]==out_row_pointers[u+1]){
        num_nodes++;
    }
    if(in_row_pointers[v]==in_row_pointers[v+1]){
        num_nodes++;
    }

    // 增加出边
    uint32_t out_start = out_row_pointers[u];
    uint32_t out_end = out_row_pointers[u + 1];
    uint32_t out_degree = out_end - out_start;

    // 找到插入位置以保持排序
    uint32_t* out_ptr = out_column_indices + out_start;
    uint32_t insert_pos = std::lower_bound(out_ptr, out_ptr + out_degree, v) - out_ptr;

    // 检查是否已经存在
    if (insert_pos < out_degree && out_ptr[insert_pos] == v) {
        // 边已存在
        return false;
    }

    // 重新分配 out_column_indices
    uint32_t* new_out_column_indices = new uint32_t[num_edges];
    std::memcpy(new_out_column_indices, out_column_indices, (out_start + insert_pos) * sizeof(uint32_t));
    new_out_column_indices[out_start + insert_pos] = v;
    std::memcpy(new_out_column_indices + out_start + insert_pos + 1, out_column_indices + out_start + insert_pos, (num_edges - (out_start + insert_pos)) * sizeof(uint32_t));
    delete[] out_column_indices;
    out_column_indices = new_out_column_indices;

    // 增加入边
    uint32_t in_start = in_row_pointers[v];
    uint32_t in_end = in_row_pointers[v + 1];
    uint32_t in_degree = in_end - in_start;

    // 找到插入位置以保持排序
    uint32_t* in_ptr = in_column_indices + in_start;
    uint32_t in_insert_pos = std::lower_bound(in_ptr, in_ptr + in_degree, u) - in_ptr;

    // 检查是否已经存在
    if (in_insert_pos < in_degree && in_ptr[in_insert_pos] == u) {
        // 边已存在
        return false;
    }

    // 重新分配 in_column_indices
    uint32_t* new_in_column_indices = new uint32_t[num_edges + 1];
    std::memcpy(new_in_column_indices, in_column_indices, (in_start + in_insert_pos) * sizeof(uint32_t));
    new_in_column_indices[in_start + in_insert_pos] = u;
    std::memcpy(new_in_column_indices + in_start + in_insert_pos + 1, in_column_indices + in_start + in_insert_pos, (num_edges - (in_start + in_insert_pos)) * sizeof(uint32_t));
    delete[] in_column_indices;
    in_column_indices = new_in_column_indices;

    // 更新 row_pointers
    for (uint32_t i = u + 1; i <= max_node_id; ++i) {
        out_row_pointers[i]++;
    }
    for (uint32_t i = v + 1; i <= max_node_id; ++i) {
        in_row_pointers[i]++;
    }



    num_edges++;

    return true;
}

// 删除边
bool CSRGraph::removeEdge(uint32_t u, uint32_t v) {
    if (u > max_node_id || v > max_node_id) return false;

    // 删除出边
    uint32_t out_start = out_row_pointers[u];
    uint32_t out_end = out_row_pointers[u + 1];
    uint32_t* out_ptr = out_column_indices + out_start;
    uint32_t* out_found = std::find(out_ptr, out_ptr + (out_end - out_start), v);
    if (out_found == out_ptr + (out_end - out_start)) {
        // 边不存在
        return false;
    }
    uint32_t out_index = out_found - out_column_indices;

    // 重新分配 out_column_indices
    uint32_t* new_out_column_indices = new uint32_t[num_edges - 1];
    std::memcpy(new_out_column_indices, out_column_indices, out_index * sizeof(uint32_t));
    std::memcpy(new_out_column_indices + out_index, out_column_indices + out_index + 1, (num_edges - out_index - 1) * sizeof(uint32_t));
    delete[] out_column_indices;
    out_column_indices = new_out_column_indices;

    // 删除入边
    uint32_t in_start = in_row_pointers[v];
    uint32_t in_end = in_row_pointers[v + 1];
    uint32_t* in_ptr = in_column_indices + in_start;
    uint32_t* in_found = std::find(in_ptr, in_ptr + (in_end - in_start), u);
    if (in_found == in_ptr + (in_end - in_start)) {
        // 入边不存在，逻辑错误
        return false;
    }
    uint32_t in_index = in_found - in_column_indices;

    // 重新分配 in_column_indices
    uint32_t* new_in_column_indices = new uint32_t[num_edges - 1];
    std::memcpy(new_in_column_indices, in_column_indices, in_index * sizeof(uint32_t));
    std::memcpy(new_in_column_indices + in_index, in_column_indices + in_index + 1, (num_edges - in_index - 1) * sizeof(uint32_t));
    delete[] in_column_indices;
    in_column_indices = new_in_column_indices;

    // 更新 row_pointers
    for (uint32_t i = u + 1; i <= max_node_id; ++i) {
        out_row_pointers[i]--;
    }
    for (uint32_t i = v + 1; i <= max_node_id; ++i) {
        in_row_pointers[i]--;
    }

    num_edges--;

    // 检查并更新节点的有效性
    if (out_row_pointers[u] == out_row_pointers[u + 1] && in_row_pointers[u] == in_row_pointers[u + 1]) {
        num_nodes--;
    }
    if (out_row_pointers[v] == out_row_pointers[v + 1] && in_row_pointers[v] == in_row_pointers[v + 1]) {
        num_nodes--;
    }

    // 更新 max_node_id
    while (max_node_id > 0 && out_row_pointers[max_node_id - 1] == out_row_pointers[max_node_id] &&
           in_row_pointers[max_node_id - 1] == in_row_pointers[max_node_id]) {
        max_node_id--;
    }


    return true;
}

// 修改节点的分区号
bool CSRGraph::setPartition(uint32_t node, int16_t partition) {
    if (node > max_node_id) return false;
    partitions[node] = partition;
    return true;
}

// 获取节点的分区号
int CSRGraph::getPartition(uint32_t node) const {
    if (node > max_node_id) return -1;
    return partitions[node];
}

// 打印图的基本信息
void CSRGraph::printInfo() const {
    std::cout << "Number of nodes: " << max_node_id << std::endl;
    std::cout << "Number of edges: " << num_edges << std::endl;
}


// 打印所有节点和边的信息
void CSRGraph::printAllInfo() const {
    std::cout << "CSRGraph Information:" << std::endl;
    std::cout << "Number of nodes: " << num_nodes << std::endl;    
    std::cout << "Number of edges: " << num_edges << std::endl;

    std::cout << "Edges:" << std::endl;
    for (uint32_t u = 0; u < max_node_id+1; ++u) {
        uint32_t out_start = out_row_pointers[u];
        uint32_t out_end = out_row_pointers[u + 1];
        for (uint32_t i = out_start; i < out_end; ++i) {
            uint32_t v = out_column_indices[i];
            std::cout << "  " << u << " -> " << v << std::endl;
        }
    }

    std::cout << "Node and Partitions:" << std::endl;
    for (uint32_t i = 0; i < max_node_id+1; ++i) {
        if(!nodeExist(i))continue;
        std::cout << "  Node " << i << ": Partition " << partitions[i] << std::endl;
    }
    std::cout << "Outgoing edges:" << std::endl;
    for (uint32_t u = 0; u < max_node_id+1; ++u) {
        uint32_t out_start = out_row_pointers[u];
        uint32_t out_end = out_row_pointers[u + 1];
        if(out_end - out_start == 0)continue;
        std::cout << "  Node " << u << " has " << (out_end - out_start) << " outgoing edges: ";
        for (uint32_t i = out_start; i < out_end; ++i) {
            std::cout << out_column_indices[i] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Incoming edges:" << std::endl;
    for (uint32_t u = 0; u < max_node_id+1; ++u) {
        uint32_t in_start = in_row_pointers[u];
        uint32_t in_end = in_row_pointers[u + 1];
        if(in_end - in_start == 0)continue;
        std::cout << "  Node " << u << " has " << (in_end - in_start) << " incoming edges: ";
        for (uint32_t i = in_start; i < in_end; ++i) {
            std::cout << in_column_indices[i] << " ";
        }
        std::cout << std::endl;
    }

}

void CSRGraph::printCSRs() const{
    
    std::cout<<std::endl;
    std::cout << "out_row_pointers: ";
    for (uint32_t i = 0; i <= max_node_id+1; ++i) {
        std::cout << out_row_pointers[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "in_row_pointers: ";
    for (uint32_t i = 0; i <= max_node_id+1; ++i) {
        std::cout << in_row_pointers[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "out_column_indices: ";
    for (uint32_t i = 0; i < num_edges; ++i) {
        std::cout << out_column_indices[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "in_column_indices: ";
    for (uint32_t i = 0; i < num_edges; ++i) {
        std::cout << in_column_indices[i] << " ";
    }
    std::cout<<std::endl;

}


uint64_t CSRGraph::getMemoryUsage() const
{
    uint64_t memoryUsage = 0;
    // 计算出边和入边的列索引数组所占内存
    memoryUsage += num_edges * sizeof(uint32_t) * 2; // out_column_indices 和 in_column_indices

    // 计算出边和入边的行指针数组所占内存
    memoryUsage += (max_node_id + 2) * sizeof(uint32_t) * 2; // out_row_pointers 和 in_row_pointers

    // 计算分区数组所占内存
    memoryUsage += max_node_id * sizeof(int16_t); // partitions

    return memoryUsage;
}

bool CSRGraph::nodeExist(uint32_t node) const
{
    if(node > max_node_id)return false;
    if(out_row_pointers[node] == out_row_pointers[node + 1] && in_row_pointers[node] == in_row_pointers[node + 1])return false;
    return true;
}

uint32_t CSRGraph::getNodesNum() const
{   
    uint32_t num=0;
    for (uint32_t i = 0; i < max_node_id + 1; i++)
    {
        if(out_row_pointers[i]==out_row_pointers[i+1]&&in_row_pointers[i]==in_row_pointers[i+1])
            continue;
        else num++;
    }
    
    return num;
}
