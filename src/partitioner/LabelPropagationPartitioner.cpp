#include "partitioner/LabelPropagationPartitioner.h"
#include "CSR.h"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
void LabelPropagationPartitioner::partition(Graph& graph, PartitionManager& partition_manager) {
    // Step 1: 构造CSRGraph
    CSRGraph csr;
    if (!csr.fromGraph(graph)) {
        // 处理构造失败
        return;
    }

    // Step 2: 初始化分区数组
    std::vector<int> partitions(csr.max_node_id+1, -1);
    std::mutex partition_mutex;

    // Step 3: 实现最小割算法（标签传播）
    auto label_propagation = [&](int thread_id, int num_threads) {
        bool changed = true;
        while (changed) {
            changed = false;
            for (uint32_t node = thread_id; node < csr.max_node_id; node += num_threads) {
                if (csr.getOutDegree(node) == 0 && csr.getInDegree(node) == 0)
                    continue; // 忽略无连接的节点
                std::unordered_map<int, int> label_counts;
                // 统计邻居的分区
                uint32_t out_degree = csr.getOutDegree(node);
                uint32_t in_degree = csr.getInDegree(node);
                for (uint32_t i = 0; i < out_degree; ++i) {
                    int neighbor = csr.out_column_indices[csr.out_row_pointers[node] + i];
                    if (partitions[neighbor] != -1)
                        label_counts[partitions[neighbor]]++;
                }
                for (uint32_t i = 0; i < in_degree; ++i) {
                    int neighbor = csr.in_column_indices[csr.in_row_pointers[node] + i];
                    if (partitions[neighbor] != -1)
                        label_counts[partitions[neighbor]]++;
                }
                // 找到最多的标签
                int max_count = 0;
                int best_label = partitions[node];
                for (const auto& pair : label_counts) {
                    if (pair.second > max_count) {
                        max_count = pair.second;
                        best_label = pair.first;
                    }
                }
                // 更新标签
                if (best_label != partitions[node]) {
                    {
                        std::lock_guard<std::mutex> lock(partition_mutex);
                        partitions[node] = best_label;
                    }
                    changed = true;
                }
            }
        }
    };

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    // 初始化分区
    for (uint32_t node = 0; node < csr.max_node_id; ++node) {
        if (csr.getOutDegree(node) > 0 || csr.getInDegree(node) > 0) {
            partitions[node] = node;
        }
    }

    // 启动线程进行标签传播
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(label_propagation, i, num_threads);
    }

    for (auto& th : threads) {
        th.join();
    }

    // 打印分区结果
    for (uint32_t node = 0; node < csr.max_node_id; ++node) {
        if(partitions[node] != -1)
            std::cout << "Node " << node << " is in partition " << partitions[node] << std::endl;
    }
    // // Step 4: 更新PartitionManager
    // 统计每个分区的节点数量
    std::unordered_map<int, int> partition_counts;
    for (const auto& partition : partitions) {
        if (partition != -1) {
            partition_counts[partition]++;
        }
    }

    // 打印每个分区的节点数量
    for (const auto& pair : partition_counts) {
        std::cout << "Partition " << pair.first << " has " << pair.second << " nodes." << std::endl;
    }
    // for (int node = 0; node < csr.num_nodes; ++node) {
    //     if (partitions[node] != -1) {
    //         partition_manager.setPartition(node, partitions[node]);
    //     }
    // }
}