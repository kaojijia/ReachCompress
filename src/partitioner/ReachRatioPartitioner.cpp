#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <stack>
#include <utility>
#include <set>
#include <cmath>
#include <random>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include "graph.h"
#include "partitioner/ReachRatioPartitioner.h"
using namespace std;





const int num_threads = 50;               // 固定线程数
const double beta = 5;                    // 惩罚倍率
const size_t maxHistorySize = 1000;       // 用于求平均波动的数组大小
const double fluctuationThreshold = 1e-6; // 平均波动小于阈值就退出
const int maxIterations = 1;              // 最大迭代轮数


const int max_depth = 3;                  // 最大深度


// 获取当前时间戳
std::string getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
    return ss.str();
}

// 使用固定数量线程的 BFS 做可达点对数量查询
void ReachRatioPartitioner::computeReachability_BFS(const Graph &current_graph, const std::vector<int> &nodes, std::vector<int> &reachableSizes, int partition)
{
    int partition_id = partition;

    std::vector<std::thread> threads(num_threads);
    std::mutex mtx;
    auto bfs = [&](int node)
    {
        std::vector<bool> visited(current_graph.vertices.size(), false);
        std::queue<int> q;
        q.push(node);
        visited[node] = true;
        int reachable_count = 0;

        while (!q.empty())
        {
            int u = q.front();
            q.pop();
            reachable_count++;

            for (const auto &v : current_graph.vertices[u].LOUT)
            {
                if (current_graph.vertices[v].partition_id == partition_id && !visited[v])
                {
                    visited[v] = true;
                    q.push(v);
                }
            }
        }

        {
            // std::lock_guard<std::mutex> lock(mtx);
            reachableSizes[node] = reachable_count;
            // reachableSets[node] = new int[reachable_count];
            // int idx = 0;
            // for (int i = 0; i < visited.size(); ++i)
            // {
            //     if (visited[i])
            //     {
            //         reachableSets[node][idx++] = i;
            //     }
            // }
        }
    };

    for (size_t j = 0; j < nodes.size(); j += num_threads)
    {
        int current_num_thread = std::min(num_threads, static_cast<int>(nodes.size() - j));
        for (int i = 0; i < current_num_thread; ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
            threads[i] = std::thread(bfs, nodes[j + i]);
        }

        for (int i = 0; i < current_num_thread; ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }
    }
}

// 使用拓扑排序来计算可达数组数量
void ReachRatioPartitioner::computeReachability(const Graph &current_graph,
                                                const std::vector<int> &nodes,
                                                std::vector<int *> &reachableSets,
                                                std::vector<int> &reachableSizes,
                                                int partition)
{

    int partition_id = partition;
    for (int i = 0; i < current_graph.vertices.size(); i++)
    {
        if (current_graph.vertices[i].partition_id == partition_id)
        {
            reachableSets[i] = new int[1]; // 动态数组初始化
            reachableSets[i][0] = i;       // 初始集合仅包含自身
            reachableSizes[i] = 1;
        }
    }
    // 拓扑排序：仅对当前分区的节点排序
    auto topoOrder = topologicalSort(current_graph, nodes);

    // 按拓扑排序逆序合并可达集合
    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it)
    {
        int u = *it; // 当前节点

        // 检查索引合法性
        if (u < 0 || u >= current_graph.vertices.size())
        {
            continue;
        }
        if (current_graph.vertices[u].partition_id == -1)
        {
            continue;
        }
        if (current_graph.vertices[u].LIN.empty() && current_graph.vertices[u].LOUT.empty())
        {
            continue;
        }

        std::set<int> mergedSet;

        // 包含自身
        mergedSet.insert(u);

        // 遍历当前节点的后继节点
        for (int v : current_graph.vertices[u].LOUT)
        {
            // 限制只处理当前分区的节点
            if (current_graph.vertices[u].partition_id == current_graph.vertices[v].partition_id)
            {
                // 合并后继节点的可达集合
                for (int i = 0; i < reachableSizes[v]; ++i)
                {
                    mergedSet.insert(reachableSets[v][i]);
                }
            }
        }

        // 将合并后的集合存储到动态数组中
        if (reachableSets[u] != nullptr)
        {
            delete[] reachableSets[u];  // 释放旧内存
            reachableSets[u] = nullptr; // 防止悬挂指针
        }

        reachableSizes[u] = mergedSet.size();         // 更新大小
        reachableSets[u] = new int[mergedSet.size()]; // 分配新内存

        size_t idx = 0;
        for (int val : mergedSet)
        {
            reachableSets[u][idx++] = val;
        }

        // 输出调试信息
        std::cout << "Node " << u << " reachable set size: " << reachableSizes[u] << std::endl;
    }
}

// 拓扑排序：对指定的节点集合进行排序
std::vector<int> ReachRatioPartitioner::topologicalSort(const Graph &graph, const std::vector<int> &nodes)
{
    // 初始化入度
    std::unordered_map<int, int> inDegree;
    for (int node : nodes)
    {
        inDegree[node] = 0; // 初始化入度为 0
    }
    for (int node : nodes)
    {
        for (int neighbor : graph.vertices[node].LOUT)
        {
            if (inDegree.find(neighbor) != inDegree.end())
            {
                inDegree[neighbor]++;
            }
        }
    }


    // 初始化队列和拓扑顺序
    std::queue<int> q;
    for (int node : nodes)
    {
        if (inDegree[node] == 0)
        {
            q.push(node);
        }
    }

    std::vector<int> topoOrder;

    // 拓扑排序
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        topoOrder.push_back(u);

        for (int v : graph.vertices[u].LOUT)
        {
            if (inDegree.find(v) != inDegree.end())
            {
                --inDegree[v];
                // std::cout << "Reduced in-degree of node " << v << " to " << inDegree[v] << std::endl;
                if (inDegree[v] == 0)
                {
                    q.push(v);
                }
            }
        }
    }

    // 检查是否存在环
    if (topoOrder.size() != nodes.size())
    {
        std::cerr << "Error: Graph contains a cycle or incomplete input. Topological sorting failed." << std::endl;
        exit(EXIT_FAILURE);
    }

    return topoOrder;
}

double ReachRatioPartitioner::computeFirstTerm(const Graph &graph)
{
    std::unordered_map<int, std::vector<int>> partitionNodes;

    // 收集每个分区的节点
    for (size_t i = 0; i < graph.vertices.size(); ++i)
    {
        if (graph.vertices[i].partition_id != -1)
        { // 忽略未分配分区的节点
            partitionNodes[graph.vertices[i].partition_id].push_back(i);
        }
    }

    double firstTerm = 0.0;

    // 遍历每个分区
    for (const auto &[partition, nodes] : partitionNodes)
    {
        if (nodes.size() == 1)
        {
            continue;
        }
        std::vector<int *> reachableSets(graph.vertices.size(), nullptr);
        std::vector<int> reachableSizes(graph.vertices.size(), 0);

        // 检查当前分区是否为空
        if (nodes.empty())
        {
            std::cerr << "Warning: Partition " << partition << " is empty." << std::endl;
            continue;
        }

        // 调用可达性计算模块
        computeReachability_BFS(graph, nodes, reachableSizes, partition);

        // 统计分区内部的可达点对数
        int reachablePairs = 0;
        for (size_t i = 0; i < graph.vertices.size(); ++i)
        {
            if (reachableSizes[i] > 0 || graph.vertices[i].partition_id == partition)
            {
                reachablePairs += reachableSizes[i] - 1; // 排除自身
            }
        }

        // 计算当前分区的贡献
        int V = nodes.size();
        if (V > 1)
        {
            firstTerm += static_cast<double>(reachablePairs) / (V * (V - 1));
        }

        // 释放内存，确保合法性检查
        for (size_t i = 0; i < reachableSets.size(); ++i)
        {
            if (reachableSets[i] != nullptr)
            {
                delete[] reachableSets[i];
                reachableSets[i] = nullptr; // 避免悬挂指针
            }
        }
    }

    return firstTerm;
}

double ReachRatioPartitioner::computeFirstTerm(const Graph &graph, int partition, PartitionManager &partition_manager)
{
    int V = partition_manager.mapping[partition].size();
    if (V <= 1)
        return 0;

    int length = graph.vertices.size();
    auto node_set = partition_manager.mapping.at(partition);

    std::vector<int> nodes(node_set.begin(), node_set.end());
    // std::vector<int *> reachableSets(length, nullptr);
    std::vector<int> reachableSizes(length, 0);

    // 调用可达性计算模块
    computeReachability_BFS(graph, nodes, reachableSizes, partition);

    // 统计分区内部的可达点对数
    int reachablePairs = 0;
    for (size_t i = 0; i < length; ++i)
    {
        if (reachableSizes[i] == 0 || graph.vertices[i].partition_id != partition) continue;
        reachablePairs += reachableSizes[i] - 1; // 排除自身
    }

    // 计算当前分区的贡献
    
    double firstTerm = static_cast<double>(reachablePairs) / (V * (V - 1));
    

    return firstTerm;
}

double ReachRatioPartitioner::computeSecondTerm(const Graph &graph, PartitionManager &partition_manager)
{

    Graph &partitionGraph = partition_manager.part_g;

    std::vector<int> nodes;
    for (size_t i = 0; i < partitionGraph.vertices.size(); ++i)
    {
        nodes.push_back(i); // 将分区图的节点加入集合
    }

    std::vector<int *> reachableSets(nodes.size(), nullptr);
    std::vector<int> reachableSizes(nodes.size(), 0);

    // 调用可达性计算模块
    // 分区图上的所有点分区都是1
    computeReachability_BFS(partitionGraph, nodes,  reachableSizes, 1);

    // 统计分区图上的可达点对数
    int reachablePairs = 0;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        reachablePairs += max(0, reachableSizes[i] - 1);
    }

    // 释放内存
    for (size_t i = 0; i < reachableSets.size(); ++i)
    {
        delete[] reachableSets[i];
    }

    int V = partitionGraph.get_num_vertices();
    if (V > 1)
        return static_cast<double>(reachablePairs) / (V * (V - 1));
    return 0.0;
}

void ReachRatioPartitioner::partition(Graph &graph, PartitionManager &partition_manager)
{
    partition_with_maxQ(graph, partition_manager);
}

void ReachRatioPartitioner::partition_with_maxQ(Graph &graph, PartitionManager &partition_manager)
{
    std::cout << getCurrentTimestamp() << "   Starting ReachRatioPartitioner..." << std::endl;

    std::unordered_map<int, int> partitionSizes; // 分区号 -> 分区中节点数

    // 初始化分区
    for (size_t i = 0; i < graph.vertices.size(); ++i)
    {
        if (graph.vertices[i].out_degree > 0 || graph.vertices[i].in_degree > 0)
        {
            graph.set_partition_id(i, i); // 初始分区号为节点自身编号
            partitionSizes[i] = 1;        // 每个节点独占一个分区
        }
        else
        {
            graph.set_partition_id(i, -1); // 无效节点分区号为 -1
        }
    }
    partition_manager.build_partition_graph_without_subgraph();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<double> recentQs;

    // double currentQ = computeFirstTerm(graph) - computeSecondTerm(graph, partition_manager);
    double currentQ = 0;
    recentQs.push_back(currentQ);
    double current_secondTerm = 0;

    int iterationCount = 0;
    bool improvement = true;


    while (iterationCount < maxIterations)
    {
        iterationCount++;
        std::cout << "---------------------------" << std::endl;
        std::cout << "Iteration: " << iterationCount << ", Current Q: " << currentQ << std::endl;
        improvement = false;

        std::vector<bool> joinable(graph.vertices.size(), true); // 节点是否可加入分区
        //secondTerm中只计算了targetPartition的出边,没有计算入边,也就是受影响的分区的出边
        //因此不确定入边的分区是怎么样,需要在更新的时候重算
        std::vector<bool> need_compute(graph.vertices.size(), false); // 节点是否需要重新计算


        std::vector<int> movableNodes;

        // 收集可移动节点
        //   Louvian中是对所有节点进行遍历，目前这里是对所有未分配的节点进行处理
        // for (size_t i = 0; i < graph.vertices.size(); ++i) {
        //     int partition = graph.vertices[i].partition_id;
        //     if (partition != -1 && partitionSizes[partition] == 1) {
        //         movableNodes.push_back(i);// 将分区中只有一个节点的节点加入
        //     }
        // }



        for (size_t i = 0; i < graph.vertices.size(); ++i)
        {
            if(graph.vertices[i].out_degree == 0 && graph.vertices[i].in_degree == 0) {
                joinable[i] = false;
                continue;
            }
            int partition = graph.vertices[i].partition_id;
            if (partition != -1)
            {
                auto c = computePartitionEdges(graph, partition_manager, partition);
                if(c >= 100)
                {
                    joinable[partition] = false;
                }
                movableNodes.push_back(i); // 每轮将所有顶点加入
            }else{
                joinable[i] = false;
            }
        }
        cout<<getCurrentTimestamp()<< "  可移动节点数："<<movableNodes.size()<<endl;
        cout<<getCurrentTimestamp()<< "  开始排序"<<endl;
        // std::reverse(movableNodes.begin(), movableNodes.end());
        std::sort(movableNodes.begin(), movableNodes.end(), [&](int a, int b) {
            return graph.vertices[a].out_degree + graph.vertices[a].in_degree < graph.vertices[b].out_degree + graph.vertices[b].in_degree;
        });
        cout<<getCurrentTimestamp()<< "  排序完成"<<endl;


        int count = 0;
        size_t mv_count = 0;

        for (int node : movableNodes)
        {
            mv_count = 0;
            int originalPartition = graph.get_partition_id(node);
            std::unordered_set<int> neighborPartitions;
            for (int neighbor : graph.vertices[node].LOUT)
            {
                int neighborPartition = graph.get_partition_id(neighbor);
                if (neighborPartition != -1 && neighborPartition != originalPartition)
                {
                    neighborPartitions.insert(neighborPartition);
                }
            }
            // for (int neighbor : graph.vertices[node].LIN)
            // {
            //     int neighborPartition = graph.get_partition_id(neighbor);
            //     if (neighborPartition != -1 && neighborPartition != originalPartition)
            //     {
            //         neighborPartitions.insert(neighborPartition);
            //     }
            // }

            for (int targetPartition : neighborPartitions)
            {

                // 快速过滤不可达分区
                if (!joinable[targetPartition])
                {
                    cout<< getCurrentTimestamp() << "  Partition " << targetPartition << " is not joinable." << endl;
                    continue;
                }

                // 限制移动次数
                if (++mv_count >= 100)
                {
                    cout<< getCurrentTimestamp() << "  Node " << node << " has moved too many times." << endl;
                    break;
                }

                cout << getCurrentTimestamp() << "  Trying to move node " << node << " to partition " << targetPartition << endl;
                auto oldPartition = graph.get_partition_id(node);
                
                // 节点加入分区前两个分区的a
                double old_source_a = computeFirstTerm(graph, oldPartition, partition_manager);
                double old_target_a = computeFirstTerm(graph, targetPartition, partition_manager);
                //  更新分区图
                cout << getCurrentTimestamp() << "  before update " << node << endl;
                partition_manager.update_partition_info(node, originalPartition, targetPartition);
                cout << getCurrentTimestamp() << "  after update" << endl;
                partitionSizes[originalPartition]--;
                partitionSizes[targetPartition]++;
                cout << getCurrentTimestamp() << "  before compute " << node << endl;
                // double firstTerm = computeFirstTerm(graph);

                // 节点加入分区后两个分区的a
                double new_source_a = computeFirstTerm(graph, oldPartition, partition_manager);
                double new_target_a = computeFirstTerm(graph, targetPartition, partition_manager);
                cout << getCurrentTimestamp() << "  complete first term" << endl;
                // double secondTerm = computeSecondTerm(graph, partition_manager);
                double secondTerm = computePartitionEdges(graph, partition_manager, targetPartition);          
                
                cout << getCurrentTimestamp() << "  complete second term" << endl;
                // double newQ = firstTerm - secondTerm;
                double newQ = currentQ + new_source_a + new_target_a - old_source_a - old_target_a - secondTerm + current_secondTerm;
                // cout<<getCurrentTimestamp()<< "  after compute " << node << endl;
                if (newQ > currentQ)
                {
                    cout << "Q increases, newQ: " << newQ << endl;
                    currentQ = newQ;
                    current_secondTerm = secondTerm;

                    // //更新两个数组
                    // for(auto outNeighbour:graph.vertices[node].LIN){                       
                    //     auto partition_id = graph.vertices[outNeighbour].partition_id;
                    //      //如果分区空了就不用了
                    //     if(partition_manager.mapping[partition_id].size() == 0) continue;
                    //     if (partition_id > 0 && partition_id != targetPartition)
                    //     {
                    //         need_compute[partition_id] = true;
                    //     }
                    // }
                    // need_compute[targetPartition] = false;


                    recentQs.push_back(newQ);
                    if (recentQs.size() > maxHistorySize)
                    {
                        recentQs.erase(recentQs.begin());
                    }
                    improvement = true;
                    break;
                }
                else
                {
                    cout << "Q decreases, newQ: " << newQ << endl;
                    partition_manager.update_partition_info(node, targetPartition, originalPartition);
                    partitionSizes[originalPartition]++;
                    partitionSizes[targetPartition]--;
                    // improvement = false;
                }

            }
            cout <<getCurrentTimestamp()<< "    完成统计" <<++count<< "个点"<<endl;
            // if (improvement) break;
            // std::cout << "Partition mapping:" << std::endl;
            // for (const auto& [partition, nodes] : partition_manager.get_mapping()) {
            //     if(partition == -1||nodes.size()<1) continue;
            //     std::cout << "Partition " << partition << ": ";
            //     for (int node : nodes) {
            //         std::cout << node << " ";
            //     }
            //     std::cout << std::endl;
            // }
        }

        if (recentQs.size() == maxHistorySize)
        {
            double fluctuation = 0.0;
            for (size_t i = 1; i < recentQs.size(); ++i)
            {
                fluctuation += std::abs(recentQs[i] - recentQs[i - 1]);
            }
            fluctuation /= (recentQs.size() - 1);

            if (fluctuation < fluctuationThreshold)
            {
                std::cout << "Partitioning converged after " << iterationCount << " iterations. Final Q: " << currentQ << std::endl;
                break;
            }
        }
    }
    partition_manager.build_partition_graph_without_subgraph();
    // partition_manager.save_mapping(PROJECT_ROOT_DIR"/Partitions/livejournal_rr.txt");
    partition_manager.build_subgraphs();
}

// TODO：计算点进入分区后，分区新增的边数，如果多了的话就把罚增大，让整体q下降，这样就不能进入了
double ReachRatioPartitioner::computePartitionEdges(const Graph &graph, PartitionManager &partition_manager, int partition)
{
    // cout << getCurrentTimestamp() << "  computePartitionEdges" << endl;
    // auto & pmedges = partition_manager.get_partition_adjacency(partition);
    // cout << getCurrentTimestamp() << "  partition_adjacency get" << endl;
    // if (pmedges.empty())
    // {
    //     return 0.0f;
    // }
    // uint16_t edge_num = 0;
    double secondTerm = 0.0f;
    // double tempTerm = 0.0f;

    // for(const auto& pmedge : pmedges)
    // {
    //     auto count = pmedge.second.edge_count;
    //     edge_num += count;
    //     if (count < 6)
    //     {
    //         tempTerm = 100.0f;
    //     }
    // }
    // cout << getCurrentTimestamp() << "  complete count edge" << endl;
    
    auto pair = partition_manager.get_partition_adjacency_size(partition);
    int edge_num = pair.first;
    // TODO:分段函数，超过30绝对不行，小于30缓慢增加
    if (edge_num > 30)
    {
        secondTerm = 100;
    }
    else
    {
        secondTerm = static_cast<float>(edge_num) * 0.1;
    }
    if(pair.second == 1)
    {
        secondTerm += 100;
    }

    return secondTerm;
}



void ReachRatioPartitioner::partition_with_dfs(Graph &graph, PartitionManager &partition_manager){}
// {
//     std::cout << getCurrentTimestamp() << "   Starting ReachRatioPartitioner..." << std::endl;
//     std::unordered_map<int, int> partitionSizes; // 分区号 -> 分区中节点数
//     // 初始化分区
//     for (size_t i = 0; i < graph.vertices.size(); ++i)
//     {
//         if (graph.vertices[i].out_degree > 0 || graph.vertices[i].in_degree > 0)
//         {
//             graph.set_partition_id(i, i); // 初始分区号为节点自身编号
//             partitionSizes[i] = 1;        // 每个节点独占一个分区
//         }
//         else
//         {
//             graph.set_partition_id(i, -1); // 无效节点分区号为 -1
//         }
//     }
//     partition_manager.build_partition_graph_without_subgraph();

//     std::random_device rd;
//     std::mt19937 gen(rd());
//     std::vector<double> recentQs;
//     double currentQ = 0;
//     recentQs.push_back(currentQ);
//     double current_secondTerm = 0;

//     int iterationCount = 0;
//     bool improvement = true;

//     while (iterationCount < maxIterations)
//     {
//         iterationCount++;
//         std::cout << "---------------------------" << std::endl;
//         std::cout << "Iteration: " << iterationCount << ", Current Q: " << currentQ << std::endl;
//         improvement = false;

//         std::vector<int> movableNodes;
//         for (size_t i = 0; i < graph.vertices.size(); ++i)
//         {
//             if(graph.vertices[i].out_degree == 0 && graph.vertices[i].in_degree == 0) {
//                 continue;
//             }
//             int partition = graph.vertices[i].partition_id;
//             if (partition != -1)movableNodes.push_back(i); // 每轮将所有顶点加入
//         }
//         cout<<getCurrentTimestamp()<< "  可移动节点数："<<movableNodes.size()<<endl;
//         cout<<getCurrentTimestamp()<< "  开始排序"<<endl;
//         // std::reverse(movableNodes.begin(), movableNodes.end());
//         // 度大的节点先做
//         std::sort(movableNodes.begin(), movableNodes.end(), [&](int a, int b) {
//             return graph.vertices[a].out_degree + graph.vertices[a].in_degree > graph.vertices[b].out_degree + graph.vertices[b].in_degree;
//         });
//         cout<<getCurrentTimestamp()<< "  排序完成"<<endl;


//         int count = 0;
//         size_t mv_count = 0;

//         for (int vertex: movableNodes)
//         {
//             stack<pair<u32,u32>> stack;
//             stack.push(make_pair(vertex,0));
//             unordered_set<u32> visited; 
//             vector<u32> result;

//             while(!stack.empty()){
//                 pair<u32,u32> p = stack.top();
//                 stack.pop();
//                 auto node = p.first;
//                 auto cur_depth = p.second;
//                 // 不那么深
//                 if(cur_depth > max_depth) continue;
                
//                 if(visited.find(node) != visited.end()) continue; // 如果节点已访问，跳过
                
//                 visited.insert(node); // 标记节点为已访问

//                 //TODO:计算加入节点的a
//                 //如果有提升则加入分区

//                 result.push_back(node);
//                 for(auto neighbor : graph.vertices[node].LOUT){
//                     stack.push(make_pair(neighbor, cur_depth + 1));
//                 }
//                 for(auto neighbor : graph.vertices[node].LIN){
//                     stack.push(make_pair(neighbor, cur_depth + 1));
//                 }
//             }

//             int originalPartition = graph.get_partition_id(node);
//             std::unordered_set<int> neighborPartitions;
//             for (int neighbor : graph.vertices[node].LOUT)
//             {
//                 int neighborPartition = graph.get_partition_id(neighbor);
//                 if (neighborPartition != -1 && neighborPartition != originalPartition)
//                 {
//                     neighborPartitions.insert(neighborPartition);
//                 }
//             }
//             for (int targetPartition : neighborPartitions)
//             {
//                 cout << getCurrentTimestamp() << "  Trying to move node " << node << " to partition " << targetPartition << endl;
//                 auto oldPartition = graph.get_partition_id(node);
                
//                 // 节点加入分区前两个分区的a
//                 double old_source_a = computeFirstTerm(graph, oldPartition, partition_manager);
//                 double old_target_a = computeFirstTerm(graph, targetPartition, partition_manager);
//                 //  更新分区图
//                 cout << getCurrentTimestamp() << "  before update " << node << endl;
//                 partition_manager.update_partition_info(node, originalPartition, targetPartition);
//                 cout << getCurrentTimestamp() << "  after update" << endl;
//                 partitionSizes[originalPartition]--;
//                 partitionSizes[targetPartition]++;
//                 cout << getCurrentTimestamp() << "  before compute " << node << endl;
//                 // double firstTerm = computeFirstTerm(graph);

//                 // 节点加入分区后两个分区的a
//                 double new_source_a = computeFirstTerm(graph, oldPartition, partition_manager);
//                 double new_target_a = computeFirstTerm(graph, targetPartition, partition_manager);
//                 cout << getCurrentTimestamp() << "  complete first term" << endl;
//                 // double secondTerm = computeSecondTerm(graph, partition_manager);
//                 double secondTerm = computePartitionEdges(graph, partition_manager, targetPartition);          
                
//                 cout << getCurrentTimestamp() << "  complete second term" << endl;
//                 // double newQ = firstTerm - secondTerm;
//                 double newQ = currentQ + new_source_a + new_target_a - old_source_a - old_target_a - secondTerm + current_secondTerm;
//                 // cout<<getCurrentTimestamp()<< "  after compute " << node << endl;
//                 if (newQ > currentQ)
//                 {
//                     cout << "Q increases, newQ: " << newQ << endl;
//                     currentQ = newQ;
//                     current_secondTerm = secondTerm;
//                     recentQs.push_back(newQ);
//                     if (recentQs.size() > maxHistorySize)
//                     {
//                         recentQs.erase(recentQs.begin());
//                     }
//                     improvement = true;
//                     break;
//                 }
//                 else
//                 {
//                     cout << "Q decreases, newQ: " << newQ << endl;
//                     partition_manager.update_partition_info(node, targetPartition, originalPartition);
//                     partitionSizes[originalPartition]++;
//                     partitionSizes[targetPartition]--;
//                 }

//             }
//             cout <<getCurrentTimestamp()<< "    完成统计" <<++count<< "个点"<<endl;

//         }

//         if (recentQs.size() == maxHistorySize)
//         {
//             double fluctuation = 0.0;
//             for (size_t i = 1; i < recentQs.size(); ++i)
//             {
//                 fluctuation += std::abs(recentQs[i] - recentQs[i - 1]);
//             }
//             fluctuation /= (recentQs.size() - 1);

//             if (fluctuation < fluctuationThreshold)
//             {
//                 std::cout << "Partitioning converged after " << iterationCount << " iterations. Final Q: " << currentQ << std::endl;
//                 break;
//             }
//         }
//     }
//     partition_manager.build_partition_graph_without_subgraph();
//     // partition_manager.save_mapping(PROJECT_ROOT_DIR"/Partitions/livejournal_rr.txt");
//     partition_manager.build_subgraphs();
// }

vector<u32> ReachRatioPartitioner::dfs_find_reachable_partitions(
    Graph& graph,
    u32 depth, 
    u32 vertex, 
    map<u32, PartitionReachable> *partition_reachable
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


        //标记分区可达性
        if(graph.vertices[node].partition_id != graph.vertices[vertex].partition_id && graph.vertices[node].partition_id != -1){
            if(!(*partition_reachable)[node].out  && bibfs_->reachability_query(vertex, node)){
                (*partition_reachable)[node].out = true;
            }
            if(!(*partition_reachable)[node].in && bibfs_->reachability_query(node, vertex)){
                (*partition_reachable)[node].in = true;
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


vector<u32> ReachRatioPartitioner::dfs_partition(
    Graph& graph,
    u32 depth, 
    u32 vertex
)
{
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

        //TODO:计算加入节点的a
        //如果有提升则加入分区

        result.push_back(node);
        for(auto neighbor : graph.vertices[node].LOUT){
            stack.push(make_pair(neighbor, cur_depth + 1));
        }
        for(auto neighbor : graph.vertices[node].LIN){
            stack.push(make_pair(neighbor, cur_depth + 1));
        }
    }
}


// void ReachRatioPartitioner::partition_with_dfs(Graph &graph, PartitionManager &partition_manager)
// {
//     this->bibfs_csr_ = make_shared<BiBFSCSR>(graph);
//     this->csr_ = bibfs_csr_->getCSR();
//     this->bibfs_ = make_shared<BidirectionalBFS>(graph);
//     // this->graph_ = &graph;

//     //记录每个节点的分区
//     partitions.resize(csr_->max_node_id, -1);
//     //用来访问的节点
//     vector<int> nodes(csr_->num_nodes, 0);
//     //已经分好区的节点
//     vector<u32> partition_visited(csr_->max_node_id+1, 0);
//     for(int i = 0,count = 0; i < csr_->max_node_id+1; i++){
//         if(csr_->getOutDegree(i) == 0 && csr_->getInDegree(i) == 0) continue;
//         partitions[i] = i;
//         nodes[count++] = i;
//     } 

//     //按照度降序排序
//     sort(nodes.begin(), nodes.end(), [&](int a, int b) {
//         return csr_->getOutDegree(a)+csr_->getInDegree(a) > csr_->getOutDegree(b)+csr_->getInDegree(b);
//     });

//     //遍历每个节点
//     for(auto node : nodes){
//         auto visited = dfs_partition(graph, max_depth, node);
//         cout << "Partition " << node << " has nodes: ";
//         for(auto v : visited){
//             // graph.vertices[v].partition_id = node;
//             cout<< v << " ";
//         }
//         cout << endl;
//     }
//     cout<<endl;
// }


// vector<u32> ReachRatioPartitioner::dfs(
//     u32 depth, 
//     u32 vertex, 
//     map<u32, PartitionReachable> *partition_reachable
//     //分区和信息的记录

// ){
//     stack<pair<u32,u32>> stack;
//     stack.push(make_pair(vertex,0));
//     unordered_set<u32> visited; 
//     vector<u32> result;

//     while(!stack.empty()){
//         pair<u32,u32> p = stack.top();
//         stack.pop();
//         auto node = p.first;
//         auto cur_depth = p.second;
//         if(cur_depth > depth) continue;
//         if(visited.find(node) != visited.end()) continue; // 如果节点已访问，跳过
//         visited.insert(node); // 标记节点为已访问
//         result.push_back(node);

//         //如果要找是否有可达分区的话，在这里存入的分区号和可达信息
//         if(partition_reachable != nullptr){
//             if(csr_->getPartition(node) == csr_->getPartition(vertex))continue;
//             if(bibfs_->reachability_query(vertex, node)){
//                 (*partition_reachable)[node].out = true;
//             }
//             if(bibfs_->reachability_query(node, vertex)){
//                 (*partition_reachable)[node].in = true;
//             }
//         }

//         u32 degree = 0;
//         auto out_neighbors = csr_->getOutgoingEdges(node, degree);
//         for(auto i = 0; i < degree; i++){
//             stack.push(make_pair(out_neighbors[i], cur_depth + 1));
//         }

//         degree = 0;
//         auto in_neighbors = csr_->getIncomingEdges(node, degree);
//         for(auto i = 0; i < degree; i++){
//             stack.push(make_pair(in_neighbors[i], cur_depth + 1));
//         }
//     }

//     return result;
// }