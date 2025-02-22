#include "BidirectionalBFS.h"
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
using namespace std;

// 构造函数，传入图并构建邻接表和逆邻接表
BidirectionalBFS::BidirectionalBFS(Graph& graph) : g(graph){
    //this->csr.fromGraph(g);
    buildAdjList();  // 构建邻接表和逆邻接表
}

// 初始化邻接表和逆邻接表
void BidirectionalBFS::buildAdjList() {
    adjList.resize(g.vertices.size());
    reverseAdjList.resize(g.vertices.size());  // 初始化逆邻接表

    for (int node = 0; node < g.vertices.size(); ++node) {
        const auto& neighbors = g.vertices[node].LOUT;  // 使用LOUT构建邻接表
        for (int neighbor : neighbors) {
            adjList[node].push_back(neighbor);          // 邻接表
            reverseAdjList[neighbor].push_back(node);   // 逆邻接表，反向连接
        }
    }
}


void BidirectionalBFS::offline_industry()
{
    this->buildAdjList();
    return;
}

// 双向BFS查询
bool BidirectionalBFS::reachability_query(int source, int target) {
    if (source == target) return true;
    if (source >= adjList.size() || target >= adjList.size() || source < 0 || target < 0) {
        return false; // 如果超出范围，直接返回不可达
    }
        // 检查是否为孤立节点（无出边且无入边）
    if (adjList[source].empty() && reverseAdjList[source].empty()) return false;
    if (adjList[target].empty() && reverseAdjList[target].empty()) return false;
    
    // BFS队列
    std::queue<int> queueFromSource, queueFromTarget;
    std::unordered_set<int> visitedFromSource, visitedFromTarget;

    // 初始化
    queueFromSource.push(source);
    visitedFromSource.insert(source);

    queueFromTarget.push(target);
    visitedFromTarget.insert(target);

    // 开始双向BFS
    while (!queueFromSource.empty() && !queueFromTarget.empty()) {
        // 从source侧扩展一步
        if (bfsStep(queueFromSource, visitedFromSource, visitedFromTarget, adjList)) {
            return true;
        }

        // 从target侧扩展一步，使用逆邻接表
        if (bfsStep(queueFromTarget, visitedFromTarget, visitedFromSource, reverseAdjList)) {
            return true;
        }
    }

    return false;
}

// 单次BFS步进，扩展一层
bool BidirectionalBFS::bfsStep(std::queue<int>& queue, std::unordered_set<int>& visited, 
                               std::unordered_set<int>& opposite_visited, 
                               const std::vector<std::vector<int>>& adjList) {
    if (queue.empty()) return false;

    int current = queue.front();
    queue.pop();

    // 检查邻接表有效性
    if (current >= adjList.size()) return false;

    // 获取邻居节点
    const auto& neighbors = adjList[current];
    for (int neighbor : neighbors) {
        // 如果在对方的访问集合中，说明路径相遇
        if (opposite_visited.find(neighbor) != opposite_visited.end()) {
            return true;
        }

        // 如果未访问过该节点，则加入访问队列
        if (visited.find(neighbor) == visited.end()) {
            visited.insert(neighbor);
            queue.push(neighbor);
        }
    }

    return false;
}

// 必须指定分区号，否则无法进行分区内搜索
std::vector<int> BidirectionalBFS::findPath(int source, int target, int partition_number) {
    if (source == target) {
        return {source};
    }

    if (source >= g.vertices.size() || target >= g.vertices.size() || source < 0 || target < 0) {
        return {};
    }
    if (g.vertices[source].LOUT.empty() || g.vertices[target].LIN.empty()){
        return {};
    }

    for (auto v: (g.vertices[source].LOUT)){
        if (v == target){
            return {source,target};
        }
    }


    // 初始化队列和访问集合，以及父节点记录
    std::queue<int> forward_queue, backward_queue;
    std::unordered_set<int> forward_visited, backward_visited;
    std::unordered_map<int, int> forward_parent, backward_parent;

    forward_queue.push(source);
    forward_visited.insert(source);
    forward_parent[source] = -1;  // 起点的父节点设为 -1

    backward_queue.push(target);
    backward_visited.insert(target);
    backward_parent[target] = -1; // 终点的父节点设为 -1

    int meeting_node = -1; // 记录正反搜索相遇的节点

    while (!forward_queue.empty() && !backward_queue.empty()) {
        // 正向搜索一步
        int forward_current = forward_queue.front();
        forward_queue.pop();

        for (int neighbor : adjList[forward_current]) {
            // 如果指定了分区，且邻居节点不在指定分区内，跳过
            if (partition_number != -1 && g.get_partition_id(neighbor) != partition_number) {
                continue;
            }

            if (forward_visited.find(neighbor) == forward_visited.end()) {
                forward_visited.insert(neighbor);
                forward_parent[neighbor] = forward_current;
                forward_queue.push(neighbor);

                // 检查是否在反向已访问集合中
                if (backward_visited.find(neighbor) != backward_visited.end()) {
                    meeting_node = neighbor; // 记录相遇节点
                    break; // 跳出循环
                }
            }
        }

        if (meeting_node != -1) {
            break; // 跳出主循环
        }

        // 反向搜索一步
        int backward_current = backward_queue.front();
        backward_queue.pop();

        for (int neighbor : reverseAdjList[backward_current]) {
            // 如果指定了分区，且邻居节点不在指定分区内，跳过
            if (partition_number != -1 && g.get_partition_id(neighbor) != partition_number) {
                continue;
            }

            if (backward_visited.find(neighbor) == backward_visited.end()) {
                backward_visited.insert(neighbor);
                backward_parent[neighbor] = backward_current;
                backward_queue.push(neighbor);

                // 检查是否在正向已访问集合中
                if (forward_visited.find(neighbor) != forward_visited.end()) {
                    meeting_node = neighbor; // 记录相遇节点
                    break; // 跳出循环
                }
            }
        }

        if (meeting_node != -1) {
            break; // 跳出主循环
        }
    }

    if (meeting_node == -1) {
        // 未找到路径
        return {};
    }

    // 构建完整路径
    std::vector<int> path;

    // 从相遇节点回溯到源节点
    int node = meeting_node;
    std::vector<int> forward_path;
    while (node != -1) {
        forward_path.push_back(node);
        node = forward_parent[node];
    }
    std::reverse(forward_path.begin(), forward_path.end());

    // 从相遇节点回溯到目标节点
    node = backward_parent[meeting_node];
    std::vector<int> backward_path;
    while (node != -1) {
        backward_path.push_back(node);
        node = backward_parent[node];
    }

    // 合并路径，避免重复相遇节点
    path.insert(path.end(), forward_path.begin(), forward_path.end());
    path.insert(path.end(), backward_path.begin(), backward_path.end());

    return path;
}


// void BidirectionalBFS::set_reachable_matrix()
// {   
//     uint32_t size_nodes = g.vertices.size();
//     uint32_t count = 0;
//     vector<vector<bool>> visited(g.vertices.size(),vector<bool>(g.vertices.size(),false));
//     for(int i=0;i<g.vertices.size();i++){
//         if(g.vertices[i].LOUT.empty()) continue;
//         for(int j=0;j<g.vertices.size();j++){
//             if(g.vertices[j].LIN.empty()) continue;
//             if(!visited[i][j]){
//                 vector<int> path = findPath(i,j);
//                 for(int u=0;u<path.size()-1;u++)
//                     for(int v = u+1;v<path.size();v++){
//                         visited[path[u]][path[v]] = true;
//                         count+=1;
//                     }
//             }

//         }
//     }
//     return count;
// }
uint32_t BidirectionalBFS::count_reachable() {
    uint32_t numVertices = g.vertices.size();
    std::atomic<uint32_t> count(0); // 使用原子变量进行线程安全的累加
    size_t numThreads = std::thread::hardware_concurrency(); // 获取可用硬件线程数
    if (numThreads == 0) numThreads = 4; // 如果获取失败，默认使用 4 个线程

    // 分块处理顶点范围
    auto worker = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            if (g.vertices[i].LOUT.empty()) continue;
            for (size_t j = 0; j < numVertices; ++j) {
                if (g.vertices[j].LIN.empty()) continue;
                if (reachability_query(i, j)) {
                    count++; // 原子操作，线程安全
                }
            }
        }
    };

    // 创建线程
    std::vector<std::thread> threads;
    size_t chunkSize = (numVertices + numThreads - 1) / numThreads; // 块大小
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(static_cast<size_t>(start + chunkSize), static_cast<size_t>(numVertices));
        threads.emplace_back(worker, start, end); // 分配任务
    }

    // 等待所有线程完成
    for (auto& th : threads) {
        th.join();
    }

    return count.load(); // 返回计数结果
}