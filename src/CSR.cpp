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

    // 设置节点数量
    num_nodes = max_node + 1;
    num_edges = edges.size();

    // 分配内存
    out_row_pointers = new uint32_t[num_nodes + 1];
    in_row_pointers = new uint32_t[num_nodes + 1];
    partitions = new int16_t[num_nodes];
    std::memset(out_row_pointers, 0, (num_nodes + 1) * sizeof(uint32_t));
    std::memset(in_row_pointers, 0, (num_nodes + 1) * sizeof(uint32_t));
    for (uint32_t i = 0; i < num_nodes; ++i) {
        partitions[i] = -1;
    }
    out_column_indices = new uint32_t[num_edges];
    in_column_indices = new uint32_t[num_edges];

    // 统计出边和入边的数量
    std::vector<uint32_t> out_counts(num_nodes, 0);
    std::vector<uint32_t> in_counts(num_nodes, 0);
    for (const auto& edge : edges) {
        out_counts[edge.first]++;
        in_counts[edge.second]++;
    }

    // 构建 row_pointers
    out_row_pointers[0] = 0;
    in_row_pointers[0] = 0;
    for (uint32_t i = 0; i < num_nodes; ++i) {
        out_row_pointers[i + 1] = out_row_pointers[i] + out_counts[i];
        in_row_pointers[i + 1] = in_row_pointers[i] + in_counts[i];
    }

    // 填充 column_indices
    std::vector<uint32_t> out_offsets(num_nodes, 0);
    std::vector<uint32_t> in_offsets(num_nodes, 0);
    for (const auto& edge : edges) {
        uint32_t src = edge.first;
        uint32_t dst = edge.second;
        out_column_indices[out_row_pointers[src] + out_offsets[src]] = dst;
        in_column_indices[in_row_pointers[dst] + in_offsets[dst]] = src;
        out_offsets[src]++;
        in_offsets[dst]++;
    }

    // 对每个节点的出边和入边进行排序
    for (uint32_t i = 0; i < num_nodes; ++i) {
        uint32_t out_start = out_row_pointers[i];
        uint32_t out_end = out_row_pointers[i + 1];
        std::sort(out_column_indices + out_start, out_column_indices + out_end);

        uint32_t in_start = in_row_pointers[i];
        uint32_t in_end = in_row_pointers[i + 1];
        std::sort(in_column_indices + in_start, in_column_indices + in_end);
    }

    return true;
}

// 从现有Graph类创建CSR
bool CSRGraph::fromGraph(const Graph& graph) {
    // 设置节点数量
    num_nodes = graph.vertices.size();

    // 统计总边数（假设每条边在 LOUT 中记录一次）
    num_edges = 0;
    for (size_t u = 0; u < graph.vertices.size(); u++) {
        num_edges += graph.vertices[u].LOUT.size();
    }

    // 分配内存
    out_row_pointers = new uint32_t[num_nodes + 1];
    in_row_pointers = new uint32_t[num_nodes + 1];
    partitions = new int16_t[num_nodes];
    std::memset(out_row_pointers, 0, (num_nodes + 1) * sizeof(uint32_t));
    std::memset(in_row_pointers, 0, (num_nodes + 1) * sizeof(uint32_t));
    for (uint32_t i = 0; i < num_nodes; ++i) {
        partitions[i] = graph.vertices[i].partition_id;
    }
    out_column_indices = new uint32_t[num_edges];
    in_column_indices = new uint32_t[num_edges];

    // 统计出边和入边的数量
    std::vector<uint32_t> out_counts(num_nodes, 0);
    std::vector<uint32_t> in_counts(num_nodes, 0);
    for (size_t u = 0; u < graph.vertices.size(); u++) {
        if(graph.vertices[u].LOUT.size()==0)continue;
        for (size_t v_idx = 0; v_idx < graph.vertices[u].LOUT.size(); v_idx++) {
            uint32_t v = graph.vertices[u].LOUT[v_idx];
            out_counts[u]++;
            if (v < num_nodes) { // 确保 v 不越界
                in_counts[v]++;
            }
        }
    }

    // 构建 row_pointers
    out_row_pointers[0] = 0;
    in_row_pointers[0] = 0;
    for (uint32_t i = 0; i < num_nodes; ++i) {
        out_row_pointers[i + 1] = out_row_pointers[i] + out_counts[i];
        in_row_pointers[i + 1] = in_row_pointers[i] + in_counts[i];
    }

    // 填充 column_indices
    std::vector<uint32_t> out_offsets(num_nodes, 0);
    std::vector<uint32_t> in_offsets(num_nodes, 0);
    for (size_t u = 0; u < graph.vertices.size(); u++) {
        if(graph.vertices[u].LOUT.size()==0)continue;
        // 对出边进行排序
        std::vector<uint32_t> sorted_LOUT(graph.vertices[u].LOUT.begin(), graph.vertices[u].LOUT.end());
        std::sort(sorted_LOUT.begin(), sorted_LOUT.end());

        for (const auto& v : sorted_LOUT) {
            if (v >= num_nodes) continue; // 确保 v 不越界

            // 添加出边
            out_column_indices[out_row_pointers[u] + out_offsets[u]] = v;
            out_offsets[u]++;

            // 添加入边
            in_column_indices[in_row_pointers[v] + in_offsets[v]] = u;
            in_offsets[v]++;
        }
    }

    // 对每个节点的入边进行排序
    for (uint32_t i = 0; i < num_nodes; ++i) {
        uint32_t in_start = in_row_pointers[i];
        uint32_t in_end = in_row_pointers[i + 1];
        std::sort(in_column_indices + in_start, in_column_indices + in_end);
    }


    // if(graph.vertices[0].LOUT.size()==0 && graph.vertices[0].LIN.size()==0){
    //     removeNode(0);
    // }
    return true;
}



// 获取某个节点的所有出边
uint32_t* CSRGraph::getOutgoingEdges(uint32_t node, uint32_t& degree) const {
    if (node >= num_nodes) {
        degree = 0;
        return nullptr;
    }
    degree = out_row_pointers[node + 1] - out_row_pointers[node];
    return out_column_indices + out_row_pointers[node];
}

// 获取某个节点的所有入边
uint32_t* CSRGraph::getIncomingEdges(uint32_t node, uint32_t& degree) const {
    if (node >= num_nodes) {
        degree = 0;
        return nullptr;
    }
    degree = in_row_pointers[node + 1] - in_row_pointers[node];
    return in_column_indices + in_row_pointers[node];
}

// 获取某个节点的出度
uint32_t CSRGraph::getOutDegree(uint32_t node) const {
    if (node >= num_nodes) return 0;
    return out_row_pointers[node + 1] - out_row_pointers[node];
}

// 获取某个节点的入度
uint32_t CSRGraph::getInDegree(uint32_t node) const {
    if (node >= num_nodes) return 0;
    return in_row_pointers[node + 1] - in_row_pointers[node];
}

// 增加节点
bool CSRGraph::addNode() {
    uint32_t new_num_nodes = num_nodes + 1;

    // 重新分配 row_pointers 和 partitions
    uint32_t* new_out_row_pointers = new uint32_t[new_num_nodes + 1];
    uint32_t* new_in_row_pointers = new uint32_t[new_num_nodes + 1];
    int16_t* new_partitions = new int16_t[new_num_nodes];

    // 复制旧数据
    std::memcpy(new_out_row_pointers, out_row_pointers, (num_nodes + 1) * sizeof(uint32_t));
    std::memcpy(new_in_row_pointers, in_row_pointers, (num_nodes + 1) * sizeof(uint32_t));
    std::memcpy(new_partitions, partitions, num_nodes * sizeof(int16_t));

    // 初始化新节点的 row_pointers 和 partition
    new_out_row_pointers[new_num_nodes] = new_out_row_pointers[num_nodes];
    new_in_row_pointers[new_num_nodes] = new_in_row_pointers[num_nodes];
    new_partitions[num_nodes] = -1;

    // 释放旧内存
    delete[] out_row_pointers;
    delete[] in_row_pointers;
    delete[] partitions;

    // 更新指针和数量
    out_row_pointers = new_out_row_pointers;
    in_row_pointers = new_in_row_pointers;
    partitions = new_partitions;
    num_nodes = new_num_nodes;

    return true;
}

// 删除节点
bool CSRGraph::removeNode(uint32_t node) {
    if (node >= num_nodes) return false;

    // 收集需要删除的出边和入边
    std::vector<std::pair<uint32_t, uint32_t>> edges_to_remove;

    // 收集出边
    uint32_t out_start = out_row_pointers[node];
    uint32_t out_end = out_row_pointers[node + 1];
    for (uint32_t i = out_start; i < out_end; ++i) {
        edges_to_remove.emplace_back(node, out_column_indices[i]);
    }

    // 收集入边
    uint32_t in_start = in_row_pointers[node];
    uint32_t in_end = in_row_pointers[node + 1];
    for (uint32_t i = in_start; i < in_end; ++i) {
        edges_to_remove.emplace_back(in_column_indices[i], node);
    }

    // 删除所有收集的边
    for (const auto& edge : edges_to_remove) {
        removeEdge(edge.first, edge.second);
    }

    // 调整 row_pointers 和 partitions
    uint32_t new_num_nodes = num_nodes - 1;

    uint32_t* new_out_row_pointers = new uint32_t[new_num_nodes + 1];
    uint32_t* new_in_row_pointers = new uint32_t[new_num_nodes + 1];
    int16_t* new_partitions = new int16_t[new_num_nodes];

    // 复制并跳过被删除的节点
    for (uint32_t i = 0; i < node; ++i) {
        new_out_row_pointers[i] = out_row_pointers[i];
        new_in_row_pointers[i] = in_row_pointers[i];
        new_partitions[i] = partitions[i];
    }

    for (uint32_t i = node; i < new_num_nodes; ++i) {
        new_out_row_pointers[i] = out_row_pointers[i + 1];
        new_in_row_pointers[i] = in_row_pointers[i + 1];
        new_partitions[i] = partitions[i + 1];
    }

    //最后一个元素用于指示最后一个节点的边的结束位置，不要有partition
    new_out_row_pointers[new_num_nodes] = out_row_pointers[num_nodes];
    new_in_row_pointers[new_num_nodes] = in_row_pointers[num_nodes];

    // 释放旧内存
    delete[] out_row_pointers;
    delete[] in_row_pointers;
    delete[] partitions;

    // 更新指针和数量
    out_row_pointers = new_out_row_pointers;
    in_row_pointers = new_in_row_pointers;
    partitions = new_partitions;
    num_nodes = new_num_nodes;

    return true;
}

// 增加边
bool CSRGraph::addEdge(uint32_t u, uint32_t v) {
    if (u >= num_nodes || v >= num_nodes) return false;

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
    uint32_t* new_out_column_indices = new uint32_t[num_edges + 1];
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
    for (uint32_t i = u + 1; i <= num_nodes; ++i) {
        out_row_pointers[i]++;
    }
    for (uint32_t i = v + 1; i <= num_nodes; ++i) {
        in_row_pointers[i]++;
    }

    num_edges++;

    return true;
}

// 删除边
bool CSRGraph::removeEdge(uint32_t u, uint32_t v) {
    if (u >= num_nodes || v >= num_nodes) return false;

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
    for (uint32_t i = u + 1; i <= num_nodes; ++i) {
        out_row_pointers[i]--;
    }
    for (uint32_t i = v + 1; i <= num_nodes; ++i) {
        in_row_pointers[i]--;
    }

    num_edges--;

    return true;
}

// 修改节点的分区号
void CSRGraph::setPartition(uint32_t node, int16_t partition) {
    if (node >= num_nodes) return;
    partitions[node] = partition;
}

// 获取节点的分区号
int16_t CSRGraph::getPartition(uint32_t node) const {
    if (node >= num_nodes) return -1;
    return partitions[node];
}

// 打印图的基本信息
void CSRGraph::printInfo() const {
    std::cout << "Number of nodes: " << num_nodes << std::endl;
    std::cout << "Number of edges: " << num_edges << std::endl;
}


// 打印所有节点和边的信息
void CSRGraph::printAllInfo() const {
    std::cout << "CSRGraph Information:" << std::endl;
    std::cout << "Number of nodes: " << num_nodes << std::endl;
    std::cout << "Number of edges: " << num_edges << std::endl;
    std::cout << "Partitions:" << std::endl;
    for (uint32_t i = 0; i < num_nodes; ++i) {
        std::cout << "  Node " << i << ": Partition " << partitions[i] << std::endl;
    }
    std::cout << "Outgoing edges:" << std::endl;
    for (uint32_t u = 0; u < num_nodes; ++u) {
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
    for (uint32_t u = 0; u < num_nodes; ++u) {
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

uint64_t CSRGraph::getMemoryUsage() const
{
    uint64_t memoryUsage = 0;
    // 计算出边和入边的列索引数组所占内存
    memoryUsage += num_edges * sizeof(uint32_t) * 2; // out_column_indices 和 in_column_indices

    // 计算出边和入边的行指针数组所占内存
    memoryUsage += (num_nodes + 1) * sizeof(uint32_t) * 2; // out_row_pointers 和 in_row_pointers

    // 计算分区数组所占内存
    memoryUsage += num_nodes * sizeof(int16_t); // partitions

    return memoryUsage;
}
