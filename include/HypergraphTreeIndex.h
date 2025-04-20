#ifndef HYPERGRAPH_TREE_INDEX_H
#define HYPERGRAPH_TREE_INDEX_H

#include "Hypergraph.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>
#include <stdexcept> // For std::out_of_range
#include <tuple>     // For std::tuple
#include <fstream>   // For file operations
#include <string>    // For std::string
#include <iomanip>   // For std::quoted
#include <sstream>   // For string stream in saveToFile
#include <thread>    // 需要包含头文件
#include <mutex>     // 需要包含头文件
#include <iostream>  // For std::cout in test
#include <chrono>    // For timing
#include <filesystem> // For directory creation
#include <numeric>   // For std::iota in recalculate
// 前向声明 HypergraphTreeIndex 类
class HypergraphTreeIndex;

// 树节点结构定义
struct TreeNode
{
    bool is_leaf = false;                            // 标记是否为叶子节点
    int intersection_size = 0;                       // 内部节点：触发此节点创建的交集大小 (瓶颈)
    std::vector<int> intersection_vertices;          // 内部节点：触发此节点创建的交集顶点列表
    int hyperedge_id = -1;                           // 叶子节点：对应的原始超边ID
    std::vector<std::shared_ptr<TreeNode>> children; // 子节点列表 (强引用)
    std::weak_ptr<TreeNode> parent;                  // 指向父节点的弱指针，避免循环引用
    int node_id = -1;                                // 节点的唯一标识符, 对应其在 nodes_ 向量中的索引
    int depth = 0;                                   // 节点在树中的深度，用于LCA计算

    // 构造函数，初始化节点ID
    TreeNode(int id) : node_id(id) {}
};

// 定义 TreeNodePtr 类型别名，方便使用共享指针管理 TreeNode
using TreeNodePtr = std::shared_ptr<TreeNode>;

// 超图树形索引类
// 通过构建一个层次化的树结构来加速超图可达性查询。
// 叶子节点代表原始超边，内部节点代表超边之间的交集。
class HypergraphTreeIndex
{
public:
    // 构造函数
    // hg: 关联的超图对象引用
    HypergraphTreeIndex(Hypergraph &hg) : hypergraph_(hg), next_node_id_(0)
    {
        size_t num_hyperedges = hypergraph_.numHyperedges();
        // 预分配内存以提高效率
        nodes_.reserve(num_hyperedges * 2);                 // 预估节点数大约是超边数的两倍
        up_.reserve(num_hyperedges * 2);                    // LCA 预计算数组
        hyperedge_to_leaf_.resize(num_hyperedges, nullptr); // 映射超边ID到叶子节点
        
        // 初始化邻居数据结构
        if (max_intersection_k_ >= 1) { // 只要 k >= 1 就需要存储
            edge_neighbors_by_size_.resize(num_hyperedges);
            for(size_t i = 0; i < num_hyperedges; ++i) {
                // 需要 k 个槽位来存储大小为 1 到 k 的邻居 (索引 0 到 k-1)
                edge_neighbors_by_size_[i].resize(max_intersection_k_);
            }
        }
    }

    // 构建索引树
    // 核心逻辑：自底向上构建聚类树。
    // 1. 为每个超边创建叶子节点。
    // 2. 计算所有叶子节点对之间的交集大小。
    // 3. 按交集大小降序排序，使用并查集（DSU）进行合并。
    // 4. 合并时创建新的内部父节点，记录交集信息。
    // 5. 最终形成一个或多个树（森林）。
    // 6. 对每个树进行 LCA（最近公共祖先）预计算。
    void buildIndex()
    {
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0)
            return; // 空超图直接返回

        // 清理旧数据
        nodes_.clear();
        roots_.clear();
        next_node_id_ = 0;
        // 重置超边到叶节点的映射
        if (hyperedge_to_leaf_.size() < num_hyperedges)
        {
            hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
        }
        else
        {
            std::fill(hyperedge_to_leaf_.begin(), hyperedge_to_leaf_.end(), nullptr);
        }
        dsu_parent_.clear();

        // 1. 创建叶子节点
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(num_hyperedges);
        for (int i = 0; i < num_hyperedges; ++i)
        {
            if (hypergraph_.getHyperedge(i).size() == 0)
                continue; // 跳过空超边

            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++); // 创建新节点
            leaf->is_leaf = true;
            leaf->hyperedge_id = i;
            nodes_.push_back(leaf); // 加入节点列表

            // 建立超边ID到叶节点的映射
            if (i < hyperedge_to_leaf_.size())
            {
                hyperedge_to_leaf_[i] = leaf;
            }
            else
            {
                // 如果 hyperedge_id 超出预分配范围，抛出异常
                throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector");
            }
            current_leaf_nodes.push_back(leaf); // 加入当前叶子节点列表
        }

        if (current_leaf_nodes.empty())
            return; // 没有有效超边
        // 2. 并行计算所有叶子节点对之间的交集
        std::vector<std::tuple<int, std::vector<int>, int, int>> merge_candidates; // (交集大小, 交集顶点, 节点1ID, 节点2ID)
        std::mutex merge_candidates_mutex;                                         // 用于保护 merge_candidates 的互斥锁
        size_t num_leaves = current_leaf_nodes.size();
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 1;
        std::vector<std::thread> threads(num_threads);

        // 预估候选对数量并预分配，减少后续合并时的重分配
        merge_candidates.reserve(num_leaves * (num_leaves - 1) / 2);

        auto calculate_leaf_intersections =
            [&](size_t start_idx, size_t end_idx)
        {
            std::vector<std::tuple<int, std::vector<int>, int, int>> local_candidates; // 线程局部结果
            // 预分配局部结果，避免频繁扩容
            local_candidates.reserve((end_idx - start_idx) * num_leaves / 2);

            for (size_t i = start_idx; i < end_idx; ++i)
            {
                for (size_t j = i + 1; j < num_leaves; ++j)
                {
                    // 注意：calculate_intersection 内部会调用 getHyperedgeIntersection，可能较慢
                    auto intersection_result = calculate_intersection(current_leaf_nodes[i], current_leaf_nodes[j]);
                    int intersection_size = intersection_result.first;
                    // 使用 std::move 移动交集顶点数据，避免拷贝
                    std::vector<int> intersection_verts = std::move(intersection_result.second);

                    if (intersection_size > 0)
                    { // 只考虑有交集的对
                        local_candidates.emplace_back(intersection_size, std::move(intersection_verts), current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                    }
                }
            }
            // 将局部结果合并到全局结果中（需要加锁）
            std::lock_guard<std::mutex> lock(merge_candidates_mutex);
            // 使用 std::move 将局部结果移动到全局结果，提高效率
            merge_candidates.insert(merge_candidates.end(),
                                    std::make_move_iterator(local_candidates.begin()),
                                    std::make_move_iterator(local_candidates.end()));
        };

        // 按第一个索引 i 划分任务
        size_t chunk_size = (num_leaves + num_threads - 1) / num_threads;
        size_t current_start = 0;
        for (unsigned int t = 0; t < num_threads; ++t)
        {
            size_t current_end = std::min(current_start + chunk_size, num_leaves);
            if (current_start >= current_end)
                break;
            threads[t] = std::thread(calculate_leaf_intersections, current_start, current_end);
            current_start = current_end;
        }

        // 等待所有线程完成
        for (unsigned int t = 0; t < threads.size(); ++t)
        {
            if (threads[t].joinable())
            {
                threads[t].join();
            }
        }

        // 3. 按交集大小降序排序合并候选，同一个交集大小的元素按交集点的字典序排序，方便归到同一个根节点
        std::sort(merge_candidates.begin(), merge_candidates.end(),
                  [](const auto &a, const auto &b)
                  {
                      if (std::get<0>(a) != std::get<0>(b))
                      {
                          return std::get<0>(a) > std::get<0>(b);
                      }
                      return std::get<1>(a) < std::get<1>(b);
                  });


        populateEdgeNeighbors(merge_candidates); // 调用修改后的填充函数

        // 初始化并查集，每个节点初始时自成一个集合
        dsu_parent_.resize(next_node_id_);
        for (int i = 0; i < next_node_id_; ++i)
        {
            dsu_parent_[i] = i;
        }

        // 4. 使用并查集合并节点，创建内部节点
        for (const auto &candidate : merge_candidates)
        {
            int size = std::get<0>(candidate);             // 交集大小
            const auto &vertices = std::get<1>(candidate); // 交集顶点
            int node1_id = std::get<2>(candidate);         // 节点1 ID
            int node2_id = std::get<3>(candidate);         // 节点2 ID

            int root1_id = find_set(node1_id); // 查找节点1所在集合的根
            int root2_id = find_set(node2_id); // 查找节点2所在集合的根

            // 如果两个节点不在同一个集合中，则合并它们
            if (root1_id != root2_id && root1_id != -1 && root2_id != -1)
            {
                TreeNodePtr root1_node = nodes_[root1_id]; // 获取根节点1的指针
                TreeNodePtr root2_node = nodes_[root2_id]; // 获取根节点2的指针

                // 新增合并逻辑：优先合并到现有内部节点（如果交集匹配）
                bool merged = false;

                // 定义一个局部 lambda 用于递归更新子树中所有节点的并查集，保证它们的根节点更新为 new_root_id
                auto updateDsu = [&](const auto &self, TreeNodePtr node, int new_root_id) -> void
                {
                    dsu_parent_[node->node_id] = new_root_id;
                    for (auto &child : node->children)
                    {
                        self(self, child, new_root_id);
                    }
                };

                if (!root1_node->is_leaf && !root2_node->is_leaf)
                {
                    // 情形 1：两个根节点都非叶且交集记录完全一致
                    if (root1_node->intersection_size == size &&
                        root1_node->intersection_vertices == vertices &&
                        root2_node->intersection_size == size &&
                        root2_node->intersection_vertices == vertices)
                    {

                        // 将 root2 的所有子节点移动到 root1 节点下
                        for (auto child : root2_node->children)
                        {
                            root1_node->children.push_back(child); // 将子节点添加到 root1 的孩子列表
                            child->parent = root1_node;            // 更新子节点的父指针
                            updateDsu(updateDsu, child, root1_id); // 更新该子树中所有节点的并查集指向
                        }
                        root2_node->children.clear();     // 清空 root2 的孩子列表
                        dsu_parent_[root2_id] = root1_id; // 更新 root2 在并查集中的指向
                        merged = true;
                    }
                    // 情形 2：两个根节点都非叶但交集记录不一致
                    else if ((root1_node->intersection_size != size || root1_node->intersection_vertices != vertices) ||
                             (root2_node->intersection_size != size || root2_node->intersection_vertices != vertices))
                    {
                        if (root1_node->intersection_size == size && root1_node->intersection_vertices == vertices)
                        {
                            // 候选交集与 root1 的记录一致，则将 root2 挂到 root1 下
                            root1_node->children.push_back(root2_node);
                            root2_node->parent = root1_node;
                            dsu_parent_[root2_id] = root1_id;
                            merged = true;
                        }
                        else if (root2_node->intersection_size == size && root2_node->intersection_vertices == vertices)
                        {
                            // 否则候选交集与 root2 的记录一致，则将 root1 挂到 root2 下
                            root2_node->children.push_back(root1_node);
                            root1_node->parent = root2_node;
                            dsu_parent_[root1_id] = root2_id;
                            merged = true;
                        }
                        else
                        {
                            // 两个根节点的交集记录都与候选交集不一致，则创建一个新的父节点
                            int parent_node_id = next_node_id_++;
                            TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                            parent->is_leaf = false;
                            parent->intersection_size = size;         // 新父节点记录候选交集大小
                            parent->intersection_vertices = vertices; // 新父节点记录候选交集顶点

                            parent->children.push_back(root1_node); // 挂上 root1
                            parent->children.push_back(root2_node); // 挂上 root2

                            // 更新子节点的父指针
                            root1_node->parent = parent;
                            root2_node->parent = parent;

                            // 将新父节点加入节点列表（确保 nodes_ 容量足够）
                            if (parent_node_id >= nodes_.size())
                            {
                                nodes_.resize(parent_node_id + 1);
                            }
                            nodes_[parent_node_id] = parent;

                            // 更新并查集：扩展 dsu_parent_ 时也要保证大小足够
                            if (parent_node_id >= dsu_parent_.size())
                            {
                                dsu_parent_.resize(parent_node_id + 1);
                            }
                            dsu_parent_[parent_node_id] = parent_node_id; // 新父节点自指
                            dsu_parent_[root1_id] = parent_node_id;
                            dsu_parent_[root2_id] = parent_node_id;

                            merged = true;
                        }
                    }
                }
                // 情形 3：只检查单个内部节点是否匹配候选交集
                // 检查root1是否是内部节点且当前交集与其记录的交集完全一致
                if (!root1_node->is_leaf &&
                    root1_node->intersection_size == size &&
                    root1_node->intersection_vertices == vertices)
                {

                    // 将root2子树合并到root1节点下
                    root1_node->children.push_back(root2_node); // 添加子节点
                    root2_node->parent = root1_node;            // 更新父指针
                    dsu_parent_[root2_id] = root1_id;           // 更新并查集
                    merged = true;
                }
                // 检查root2是否是内部节点且当前交集与其记录的交集完全一致
                else if (!root2_node->is_leaf &&
                         root2_node->intersection_size == size &&
                         root2_node->intersection_vertices == vertices)
                {

                    // 将root1子树合并到root2节点下
                    root2_node->children.push_back(root1_node); // 添加子节点
                    root1_node->parent = root2_node;            // 更新父指针
                    dsu_parent_[root1_id] = root2_id;           // 更新并查集
                    merged = true;
                }

                // 如果没有匹配的现有内部节点，则创建新父节点（保留原有逻辑）
                if (!merged)
                {
                    // 创建新的父节点（内部节点）
                    int parent_node_id = next_node_id_++;
                    TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                    parent->is_leaf = false;
                    parent->intersection_size = size;         // 记录交集大小作为瓶颈
                    parent->intersection_vertices = vertices; // 记录交集顶点
                    parent->children.push_back(root1_node);   // 添加子节点
                    parent->children.push_back(root2_node);

                    // 设置子节点的父节点指针（弱引用）
                    root1_node->parent = parent;
                    root2_node->parent = parent;

                    // 将新父节点加入节点列表
                    if (parent_node_id >= nodes_.size())
                    {
                        nodes_.resize(parent_node_id + 1); // 动态扩展节点列表
                    }
                    nodes_[parent_node_id] = parent;

                    // 更新并查集：将两个子树的根指向新的父节点
                    if (parent_node_id >= dsu_parent_.size())
                    {
                        dsu_parent_.resize(parent_node_id + 1); // 动态扩展并查集数组
                    }
                    dsu_parent_[parent_node_id] = parent_node_id; // 新父节点指向自己
                    dsu_parent_[root1_id] = parent_node_id;       // 子树根1指向新父节点
                    dsu_parent_[root2_id] = parent_node_id;       // 子树根2指向新父节点
                }
            }
        }

        // 5. 确定所有树的根节点
        roots_.clear();
        std::vector<bool> is_root_added(next_node_id_, false); // 标记根节点是否已添加
        for (int i = 0; i < next_node_id_; ++i)
        {
            // 如果节点存在且其父指针为空（说明是根节点）
            if (i < nodes_.size() && nodes_[i] && nodes_[i]->parent.expired())
            {
                int root_id = find_set(i); // 找到该树最终的根节点ID
                // 如果根节点有效且未被添加过
                if (root_id != -1 && root_id < is_root_added.size() && !is_root_added[root_id])
                {
                    if (root_id < nodes_.size() && nodes_[root_id])
                    {                                      // 再次确认根节点指针有效
                        roots_.push_back(nodes_[root_id]); // 添加到根节点列表
                        is_root_added[root_id] = true;     // 标记已添加
                    }
                }
            }
        }

        // 6. 对每棵树进行 LCA 预计算
        if (up_.size() < next_node_id_)
        {
            up_.resize(next_node_id_); // 调整 LCA 数组大小
        }
        for (int i = 0; i < next_node_id_; ++i)
        {
            // 初始化 LCA 数组，每个节点的祖先指针置空
            up_[i].assign(MAX_LCA_LOG, nullptr);
        }
        // 从每个根节点开始，递归计算深度和祖先信息
        for (const auto &root : roots_)
        {
            precompute_lca(root, 0); // 根节点深度为 0
        }
    }

    // 查询可达性
    // u, v: 原始超图中的顶点 ID
    // k: 最小交集大小阈值
    // 逻辑：
    // 1. 如果 u == v，直接返回 true。
    // 2. 获取 u 和 v 关联的所有超边（叶子节点）。
    // 3. 如果 u 和 v 共享同一个超边，直接返回 true。
    // 4. 对 u 的每个关联超边 leaf_u 和 v 的每个关联超边 leaf_v：
    //    a. 找到 leaf_u 和 leaf_v 在树中的 LCA（最近公共祖先）节点 lca_node。
    //    b. 如果 lca_node 存在且是内部节点，并且其记录的交集大小 >= k，则返回 true。
    // 5. 如果遍历完所有超边对都没有找到满足条件的 LCA，返回 false。
    bool query(int u, int v, int k)
    {
        if (u == v)
            return true; // 同一个顶点直接可达

        // 获取顶点关联的超边 ID 列表
        const auto &edges_u = hypergraph_.getIncidentHyperedges(u);
        const auto &edges_v = hypergraph_.getIncidentHyperedges(v);

        if (edges_u.empty() || edges_v.empty())
        {
            return false; // 如果任一顶点不属于任何超边，则不可达
        }

        // 检查是否共享超边（优化：避免后续复杂的 LCA 查询）
        int max_edge_id = hypergraph_.numHyperedges();
        std::vector<bool> v_edge_flags(max_edge_id, false); // 使用布尔数组快速查找
        for (int edge_id_v : edges_v)
        {
            if (edge_id_v >= 0 && edge_id_v < max_edge_id)
            {
                v_edge_flags[edge_id_v] = true;
            }
        }
        bool common_edge_found = false;
        for (int edge_id_u : edges_u)
        {
            if (edge_id_u >= 0 && edge_id_u < max_edge_id && v_edge_flags[edge_id_u])
            {
                common_edge_found = true; // 找到公共超边
                break;
            }
        }
        if (common_edge_found)
        {
            return true; // 直接可达
        }

        // 遍历所有超边对，查找 LCA
        for (int edge_id_u : edges_u)
        {
            // 获取超边 u 对应的叶子节点
            if (edge_id_u < 0 || edge_id_u >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_u])
                continue; // 跳过无效或不存在的叶节点
            TreeNodePtr leaf_u = hyperedge_to_leaf_[edge_id_u];

            for (int edge_id_v : edges_v)
            {
                // 获取超边 v 对应的叶子节点
                if (edge_id_v < 0 || edge_id_v >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_v])
                    continue; // 跳过无效或不存在的叶节点
                TreeNodePtr leaf_v = hyperedge_to_leaf_[edge_id_v];

                // 查找两个叶子节点的 LCA
                TreeNodePtr lca_node = find_lca(leaf_u, leaf_v);

                // 检查 LCA 是否满足条件
                // lca_node 必须存在，必须是内部节点（代表交集），且交集大小满足阈值 k
                if (lca_node && !lca_node->is_leaf && lca_node->intersection_size >= k)
                {
                    return true; // 找到满足条件的路径
                }
            }
        }
        return false; // 遍历完所有组合仍未找到满足条件的路径
    }

    // --- 新增：只存储交集大小的构建方法 ---
    void buildIndexSizeOnly()
    {
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0)
            return;

        // 清理旧数据 (与 buildIndex 相同)
        nodes_.clear();
        roots_.clear();
        next_node_id_ = 0;
        if (hyperedge_to_leaf_.size() < num_hyperedges)
        {
            hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
        }
        else
        {
            std::fill(hyperedge_to_leaf_.begin(), hyperedge_to_leaf_.end(), nullptr);
        }
        dsu_parent_.clear();

        // 1. 创建叶子节点 (与 buildIndex 相同)
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(num_hyperedges);
        for (int i = 0; i < num_hyperedges; ++i)
        {
            if (hypergraph_.getHyperedge(i).size() == 0)
                continue;
            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++);
            leaf->is_leaf = true;
            leaf->hyperedge_id = i;
            nodes_.push_back(leaf);
            if (i < hyperedge_to_leaf_.size())
            {
                hyperedge_to_leaf_[i] = leaf;
            }
            else
            {
                throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector");
            }
            current_leaf_nodes.push_back(leaf);
        }

        if (current_leaf_nodes.empty())
            return;

        // 2. 并行计算所有叶子节点对之间的交集大小
        // 注意：这里存储的 tuple 不再包含 std::vector<int>
        std::vector<std::tuple<int, int, int>> merge_candidates; // (交集大小, 节点1ID, 节点2ID)
        std::mutex merge_candidates_mutex;
        size_t num_leaves = current_leaf_nodes.size();
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 1;
        std::vector<std::thread> threads(num_threads);

        merge_candidates.reserve(num_leaves * (num_leaves - 1) / 2);

        auto calculate_leaf_intersections_size_only =
            [&](size_t start_idx, size_t end_idx)
        {
            // 注意：局部结果也不再包含 std::vector<int>
            std::vector<std::tuple<int, int, int>> local_candidates;
            local_candidates.reserve((end_idx - start_idx) * num_leaves / 2);

            for (size_t i = start_idx; i < end_idx; ++i)
            {
                for (size_t j = i + 1; j < num_leaves; ++j)
                {
                    // calculate_intersection 返回 pair<int, vector<int>>
                    // 我们只关心第一个元素 (size)
                    auto intersection_result = calculate_intersection(current_leaf_nodes[i], current_leaf_nodes[j]);
                    int intersection_size = intersection_result.first;

                    if (intersection_size > 0)
                    {
                        // 只存储大小和节点 ID
                        local_candidates.emplace_back(intersection_size, current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                    }
                }
            }
            std::lock_guard<std::mutex> lock(merge_candidates_mutex);
            // 直接插入，因为 tuple 不包含需要移动的大对象
            merge_candidates.insert(merge_candidates.end(), local_candidates.begin(), local_candidates.end());
        };

        size_t chunk_size = (num_leaves + num_threads - 1) / num_threads;
        size_t current_start = 0;
        for (unsigned int t = 0; t < num_threads; ++t)
        {
            size_t current_end = std::min(current_start + chunk_size, num_leaves);
            if (current_start >= current_end)
                break;
            // 使用新的 lambda 函数
            threads[t] = std::thread(calculate_leaf_intersections_size_only, current_start, current_end);
            current_start = current_end;
        }

        for (unsigned int t = 0; t < threads.size(); ++t)
        {
            if (threads[t].joinable())
            {
                threads[t].join();
            }
        }

        // 3. 按交集大小降序排序合并候选
        // 注意：排序不再需要比较顶点列表
        std::sort(merge_candidates.begin(), merge_candidates.end(),
                  [](const auto &a, const auto &b)
                  {
                      // 只按交集大小降序排列
                      return std::get<0>(a) > std::get<0>(b);
                  });



        // *** 填充邻居信息数据结构 (SizeOnly 版本) (修改点) ***
        populateEdgeNeighborsSizeOnly(merge_candidates); // 调用修改后的填充函数

        // 初始化并查集 (与 buildIndex 相同)
        dsu_parent_.resize(next_node_id_);
        for (int i = 0; i < next_node_id_; ++i)
        {
            dsu_parent_[i] = i;
        }

        // 4. 使用并查集合并节点，创建内部节点
        for (const auto &candidate : merge_candidates)
        {
            int size = std::get<0>(candidate);     // 交集大小
            int node1_id = std::get<1>(candidate); // 节点1 ID
            int node2_id = std::get<2>(candidate); // 节点2 ID

            int root1_id = find_set(node1_id);
            int root2_id = find_set(node2_id);

            if (root1_id != root2_id && root1_id != -1 && root2_id != -1)
            {
                TreeNodePtr root1_node = nodes_[root1_id];
                TreeNodePtr root2_node = nodes_[root2_id];

                // --- 合并逻辑简化：不再检查 intersection_vertices ---
                bool merged = false;
                auto updateDsu = [&](const auto &self, TreeNodePtr node, int new_root_id) -> void
                {
                    dsu_parent_[node->node_id] = new_root_id;
                    for (auto &child : node->children)
                    {
                        self(self, child, new_root_id);
                    }
                };

                // 检查是否可以合并到现有内部节点 (只比较 size)
                if (!root1_node->is_leaf && root1_node->intersection_size == size)
                {
                    // 将 root2 合并到 root1
                    root1_node->children.push_back(root2_node);
                    root2_node->parent = root1_node;
                    updateDsu(updateDsu, root2_node, root1_id); // 更新 root2 子树的 DSU
                    dsu_parent_[root2_id] = root1_id;
                    merged = true;
                }
                else if (!root2_node->is_leaf && root2_node->intersection_size == size)
                {
                    // 将 root1 合并到 root2
                    root2_node->children.push_back(root1_node);
                    root1_node->parent = root2_node;
                    updateDsu(updateDsu, root1_node, root2_id); // 更新 root1 子树的 DSU
                    dsu_parent_[root1_id] = root2_id;
                    merged = true;
                }

                // 如果没有合并到现有节点，则创建新父节点
                if (!merged)
                {
                    int parent_node_id = next_node_id_++;
                    TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                    parent->is_leaf = false;
                    parent->intersection_size = size; // 只记录交集大小
                    // parent->intersection_vertices 不再需要赋值

                    parent->children.push_back(root1_node);
                    parent->children.push_back(root2_node);
                    root1_node->parent = parent;
                    root2_node->parent = parent;

                    if (parent_node_id >= nodes_.size())
                    {
                        nodes_.resize(parent_node_id + 1);
                    }
                    nodes_[parent_node_id] = parent;

                    if (parent_node_id >= dsu_parent_.size())
                    {
                        dsu_parent_.resize(parent_node_id + 1);
                    }
                    dsu_parent_[parent_node_id] = parent_node_id;
                    dsu_parent_[root1_id] = parent_node_id;
                    dsu_parent_[root2_id] = parent_node_id;
                }
            }
        }

        // 5. 确定所有树的根节点 (与 buildIndex 相同)
        roots_.clear();
        std::vector<bool> is_root_added(next_node_id_, false);
        for (int i = 0; i < next_node_id_; ++i)
        {
            if (i < nodes_.size() && nodes_[i] && nodes_[i]->parent.expired())
            {
                int root_id = find_set(i);
                if (root_id != -1 && root_id < is_root_added.size() && !is_root_added[root_id])
                {
                    if (root_id < nodes_.size() && nodes_[root_id])
                    {
                        roots_.push_back(nodes_[root_id]);
                        is_root_added[root_id] = true;
                    }
                }
            }
        }

        // 6. 对每棵树进行 LCA 预计算 (与 buildIndex 相同)
        if (up_.size() < next_node_id_)
        {
            up_.resize(next_node_id_);
        }
        for (int i = 0; i < next_node_id_; ++i)
        {
            up_[i].assign(MAX_LCA_LOG, nullptr);
        }
        for (const auto &root : roots_)
        {
            precompute_lca(root, 0);
        }
    }

    // --- 新增：使用只存储大小的索引进行查询 ---
    // 这个函数的逻辑与原 query 函数几乎完全相同，因为原函数主要依赖 intersection_size
    bool querySizeOnly(int u, int v, int k)
    {
        if (u == v)
            return true;

        const auto &edges_u = hypergraph_.getIncidentHyperedges(u);
        const auto &edges_v = hypergraph_.getIncidentHyperedges(v);

        if (edges_u.empty() || edges_v.empty())
            return false;

        // 检查共享超边 (与 query 相同)
        int max_edge_id = hypergraph_.numHyperedges();
        std::vector<bool> v_edge_flags(max_edge_id, false);
        for (int edge_id_v : edges_v)
        {
            if (edge_id_v >= 0 && edge_id_v < max_edge_id)
            {
                v_edge_flags[edge_id_v] = true;
            }
        }
        for (int edge_id_u : edges_u)
        {
            if (edge_id_u >= 0 && edge_id_u < max_edge_id && v_edge_flags[edge_id_u])
            {
                return true; // 直接可达
            }
        }

        // 遍历所有超边对，查找 LCA (与 query 相同)
        for (int edge_id_u : edges_u)
        {
            if (edge_id_u < 0 || edge_id_u >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_u])
                continue;
            TreeNodePtr leaf_u = hyperedge_to_leaf_[edge_id_u];

            for (int edge_id_v : edges_v)
            {
                if (edge_id_v < 0 || edge_id_v >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_v])
                    continue;
                TreeNodePtr leaf_v = hyperedge_to_leaf_[edge_id_v];

                TreeNodePtr lca_node = find_lca(leaf_u, leaf_v);

                // 检查 LCA 是否满足条件 (只依赖 intersection_size)
                if (lca_node && !lca_node->is_leaf && lca_node->intersection_size >= k)
                {
                    return true; // 找到满足条件的路径
                }
            }
        }
        return false; // 未找到满足条件的路径
    }

    // Build index with caching logic moved here.
    void buildIndexCache(std::string cache_path = "")
    {
        // 1. Try loading from cache first
        bool loaded_from_cache = false;
        if (!cache_path.empty())
        {
            cache_path = cache_path+ "hypergraph_tree_index";//添加后缀
            try
            {
                loaded_from_cache = loadIndex(cache_path);
                if (loaded_from_cache)
                {
                    std::cout << "HypergraphTreeIndex loaded from cache: " << cache_path << std::endl;
                    std::cout << "Recalculating neighbor information after loading from cache..." << std::endl;
                    recalculateAndPopulateNeighbors(); // 调用修改后的重新计算函数
                    std::cout << "Neighbor information recalculated." << std::endl;
                    // LCA is recomputed inside loadIndex upon success
                    return; // Successfully loaded, no need to build
                }
                else
                {
                    // Cache miss or invalid file, proceed to build
                    std::cout << "Info: HypergraphTreeIndex cache not found or invalid at '" << cache_path << "'. Building index." << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Failed to load HypergraphTreeIndex cache '" << cache_path << "'. Building index. Error: " << e.what() << std::endl;
            }
        }

        // 2. Build the index if not loaded from cache
        std::cout << "Building HypergraphTreeIndex..." << std::endl;
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0)
        {
            std::cout << "Warning: Hypergraph has no hyperedges. Index is empty." << std::endl;
            return; // Empty hypergraph
        }

        // Clear old data before building
        clearIndexData();

        // 1. Create leaf nodes
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(num_hyperedges);
        for (int i = 0; i < num_hyperedges; ++i)
        {
            if (hypergraph_.getHyperedge(i).size() == 0)
                continue; // Skip empty hyperedges

            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++);
            leaf->is_leaf = true;
            leaf->hyperedge_id = i;
            nodes_.push_back(leaf);

            if (i < hyperedge_to_leaf_.size())
            {
                hyperedge_to_leaf_[i] = leaf;
            }
            else
            {
                throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector during build");
            }
            current_leaf_nodes.push_back(leaf);
        }

        if (current_leaf_nodes.empty())
        {
            std::cout << "Warning: No valid hyperedges found. Index is empty." << std::endl;
            return; // No valid hyperedges
        }

        // 2. Calculate intersections (parallel)
        std::vector<std::tuple<int, std::vector<int>, int, int>> merge_candidates;
        calculateAndPrepareMergeCandidates(current_leaf_nodes, merge_candidates);

        // 3. Sort merge candidates
        sortMergeCandidates(merge_candidates);


        populateEdgeNeighbors(merge_candidates);

        // 4. Initialize DSU
        initializeDSU();

        // 5. Merge nodes using DSU
        mergeNodes(merge_candidates);

        // 6. Find roots
        findRoots();

        // 7. Precompute LCA
        precomputeAllLCA(); // Use the helper function

        std::cout << "HypergraphTreeIndex build complete." << std::endl;

        // 3. Save the newly built index to cache if path is provided
        if (!cache_path.empty())
        {
            try
            {
                // Ensure cache directory exists
                std::filesystem::path p(cache_path);
                if (p.has_parent_path())
                {
                    std::filesystem::create_directories(p.parent_path());
                }
                if (saveIndex(cache_path))
                {
                    std::cout << "HypergraphTreeIndex saved to cache: " << cache_path << std::endl;
                }
                else
                {
                    std::cerr << "Warning: Failed to save HypergraphTreeIndex cache to '" << cache_path << "'." << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Failed to save HypergraphTreeIndex cache '" << cache_path << "'. Error: " << e.what() << std::endl;
            }
        }
    }

    // Build index storing only intersection size, with caching logic.
    void buildIndexCacheSizeOnly(std::string cache_path = "")
    {
        // 1. Try loading from cache first
        bool loaded_from_cache = false;
        if (!cache_path.empty())
        {
            cache_path = cache_path+ "hypergraph_tree_index_SizeOnly";//添加后缀
            try
            {
                loaded_from_cache = loadIndex(cache_path);
                if (loaded_from_cache)
                {
                    std::cout << "HypergraphTreeIndex (SizeOnly) loaded from cache: " << cache_path << std::endl;
                    std::cout << "Recalculating neighbor information (SizeOnly) after loading from cache..." << std::endl;
                    recalculateAndPopulateNeighborsSizeOnly(); // 使用 SizeOnly 版本
                    std::cout << "Neighbor information recalculated." << std::endl;
                    // LCA is recomputed inside loadIndex upon success
                    return; // Successfully loaded
                }
                else
                {
                    std::cout << "Info: HypergraphTreeIndex (SizeOnly) cache not found or invalid at '" << cache_path << "'. Building index." << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Failed to load HypergraphTreeIndex (SizeOnly) cache '" << cache_path << "'. Building index. Error: " << e.what() << std::endl;
            }
        }

        // 2. Build the size-only index if not loaded
        std::cout << "Building HypergraphTreeIndex (SizeOnly)..." << std::endl;
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0)
        {
            std::cout << "Warning: Hypergraph has no hyperedges. Index is empty." << std::endl;
            return;
        }

        // Clear old data
        clearIndexData();

        // 1. Create leaf nodes (same as buildIndex)
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(num_hyperedges);
        for (int i = 0; i < num_hyperedges; ++i)
        {
            if (hypergraph_.getHyperedge(i).size() == 0)
                continue;
            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++);
            leaf->is_leaf = true;
            leaf->hyperedge_id = i;
            nodes_.push_back(leaf);
            if (i < hyperedge_to_leaf_.size())
            {
                hyperedge_to_leaf_[i] = leaf;
            }
            else
            {
                throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector during build (SizeOnly)");
            }
            current_leaf_nodes.push_back(leaf);
        }

        if (current_leaf_nodes.empty())
        {
            std::cout << "Warning: No valid hyperedges found. Index is empty." << std::endl;
            return;
        }

        // 2. Calculate intersection sizes (parallel)
        std::vector<std::tuple<int, int, int>> merge_candidates_size_only; // (size, id1, id2)
        calculateAndPrepareMergeCandidatesSizeOnly(current_leaf_nodes, merge_candidates_size_only);

        // 3. Sort merge candidates by size
        sortMergeCandidatesSizeOnly(merge_candidates_size_only);

        // *** 填充邻居信息数据结构 (SizeOnly 版本) (修改点) ***
        populateEdgeNeighborsSizeOnly(merge_candidates_size_only); // 调用修改后的填充函数


        // 4. Initialize DSU
        initializeDSU();

        // 5. Merge nodes using DSU (size only logic)
        mergeNodesSizeOnly(merge_candidates_size_only);

        // 6. Find roots
        findRoots();

        // 7. Precompute LCA
        precomputeAllLCA();

        std::cout << "HypergraphTreeIndex (SizeOnly) build complete." << std::endl;

        // 3. Save the newly built index to cache
        if (!cache_path.empty())
        {
            try
            {
                std::filesystem::path p(cache_path);
                if (p.has_parent_path())
                {
                    std::filesystem::create_directories(p.parent_path());
                }
                if (saveIndex(cache_path))
                { // saveIndex saves the structure, which is the same format
                    std::cout << "HypergraphTreeIndex (SizeOnly) saved to cache: " << cache_path << std::endl;
                }
                else
                {
                    std::cerr << "Warning: Failed to save HypergraphTreeIndex (SizeOnly) cache to '" << cache_path << "'." << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Warning: Failed to save HypergraphTreeIndex (SizeOnly) cache '" << cache_path << "'. Error: " << e.what() << std::endl;
            }
        }
    }

    // 估算索引占用的内存（MB）
    // 累加各个成员变量（vector, shared_ptr 等）及其内容的估算大小。
    // 使用 capacity() 而非 size() 来估算 vector 占用的内存，更接近实际情况。
    double getMemoryUsageMB() const
    {
        size_t memory_bytes = 0;
        memory_bytes += sizeof(*this); // 索引对象自身的大小

        // nodes_ 向量本身 + 指针 + TreeNode 对象 + 内部 vector 数据
        memory_bytes += sizeof(nodes_);
        memory_bytes += nodes_.capacity() * sizeof(TreeNodePtr); // 存储指针的开销
        for (const auto &node_ptr : nodes_)
        {
            if (node_ptr)
            {
                memory_bytes += sizeof(TreeNode);                                         // TreeNode 对象本身
                memory_bytes += node_ptr->intersection_vertices.capacity() * sizeof(int); // intersection_vertices 数据
                memory_bytes += node_ptr->children.capacity() * sizeof(TreeNodePtr);      // children 向量中的指针
            }
        }

        // roots_ 向量本身 + 指针
        memory_bytes += sizeof(roots_);
        memory_bytes += roots_.capacity() * sizeof(TreeNodePtr);

        // hyperedge_to_leaf_ 向量本身 + 指针
        memory_bytes += sizeof(hyperedge_to_leaf_);
        memory_bytes += hyperedge_to_leaf_.capacity() * sizeof(TreeNodePtr);

        // dsu_parent_ 向量本身 + 数据
        memory_bytes += sizeof(dsu_parent_);
        memory_bytes += dsu_parent_.capacity() * sizeof(int);

        // up_ 数组 (LCA 预计算)
        memory_bytes += sizeof(up_);                                       // 外层 vector 对象
        memory_bytes += up_.capacity() * sizeof(std::vector<TreeNodePtr>); // 外层 vector 存储内层 vector 对象的开销
        for (const auto &vec : up_)
        {
            memory_bytes += sizeof(vec);                          // 内层 vector 对象本身
            memory_bytes += vec.capacity() * sizeof(TreeNodePtr); // 内层 vector 存储指针的开销
        }

        // 新增：估算 edge_neighbors_by_size_ 的内存 (修改点: 检查 k>=1)
        if (max_intersection_k_ >= 1) {
            memory_bytes += sizeof(edge_neighbors_by_size_); // 外层 vector 对象
            memory_bytes += edge_neighbors_by_size_.capacity() * sizeof(std::vector<std::vector<int>>); // 外层 vector 存储中层 vector 对象的开销
            for (const auto& mid_vec : edge_neighbors_by_size_) {
                memory_bytes += sizeof(mid_vec); // 中层 vector 对象
                memory_bytes += mid_vec.capacity() * sizeof(std::vector<int>); // 中层 vector 存储内层 vector 对象的开销
                for (const auto& inner_vec : mid_vec) {
                    memory_bytes += sizeof(inner_vec); // 内层 vector 对象
                    memory_bytes += inner_vec.capacity() * sizeof(int); // 内层 vector 存储 int 的开销
                }
            }
        }


        return static_cast<double>(memory_bytes) / (1024.0 * 1024.0); // 转换为 MB
    }

    
    // 新增：获取指定超边和交集大小的邻居列表
    // edge_id: 目标超边的 ID
    // intersection_size: 期望的交集大小 (范围 [1, max_intersection_k_]) (修改点)
    // 返回: 包含邻居超边 ID 的 const 引用；如果输入无效或无邻居，返回空 vector 的引用
    const std::vector<int>& getNeighborsBySize(int edge_id, int intersection_size) const {
        // 检查输入有效性 (修改点: intersection_size >= 1)
        if (intersection_size < 1 || intersection_size > max_intersection_k_ ||
            edge_id < 0 || edge_id >= static_cast<int>(edge_neighbors_by_size_.size()) ||
            (intersection_size - 1) < 0 || (intersection_size - 1) >= static_cast<int>(edge_neighbors_by_size_[edge_id].size()) ) {
            // 返回静态空 vector 的引用以避免创建临时对象
            static const std::vector<int> empty_vec;
            return empty_vec;
        }
        // 调整索引：大小 s 存储在索引 s-1 (修改点)
        return edge_neighbors_by_size_[edge_id][intersection_size - 1];
    }

    // 获取索引树中的总节点数
    size_t getTotalNodes() const
    {
        // next_node_id_ 记录了创建的总节点数
        return next_node_id_;
    }

private:
    Hypergraph &hypergraph_;                     // 关联的超图引用
    std::vector<TreeNodePtr> nodes_;             // 存储树中所有节点的列表 (叶子和内部节点)
    std::vector<TreeNodePtr> roots_;             // 存储森林中所有树的根节点
    std::vector<TreeNodePtr> hyperedge_to_leaf_; // 映射：超边 ID -> 对应的叶子节点指针
    int next_node_id_;                           // 用于分配新节点 ID 的计数器
    std::vector<int> dsu_parent_;                // 并查集数组，用于构建树
    static const int MAX_LCA_LOG = 20;           // LCA 预计算的最大深度（2^20 足够处理大量节点）
    std::vector<std::vector<TreeNodePtr>> up_;   // LCA 预计算数组, up_[i][j] 表示节点 i 的第 2^j 个祖先


    // --- 新增邻居信息成员 ---
    int max_intersection_k_; // 存储邻居的最大交集大小 k (>= 1) (修改点)
    // 结构: edge_neighbors_by_size_[edge_id][intersection_size - 1] = vector<neighbor_edge_id> (修改点)
    std::vector<std::vector<std::vector<int>>> edge_neighbors_by_size_;



    // 计算两个叶子节点对应超边的交集
    // n1, n2: 两个叶子节点的指针
    // 返回: pair<交集大小, 交集顶点列表>
    std::pair<int, std::vector<int>> calculate_intersection(TreeNodePtr n1, TreeNodePtr n2)
    {
        // 确保输入是有效的叶子节点
        if (!n1 || !n2 || !n1->is_leaf || !n2->is_leaf)
        {
            return {0, {}}; // 无效输入返回空交集
        }
        // 调用 Hypergraph 类的方法计算交集
        std::vector<int> intersection_verts = hypergraph_.getHyperedgeIntersection(n1->hyperedge_id, n2->hyperedge_id);
        return {static_cast<int>(intersection_verts.size()), intersection_verts};
    }

    // 并查集查找操作（带路径压缩）
    // node_id: 要查找的节点 ID
    // 返回: 节点所在集合的根节点 ID，如果 node_id 无效则返回 -1
    int find_set(int node_id)
    {
        if (node_id < 0 || node_id >= dsu_parent_.size())
        {
            return -1; // 无效 ID
        }
        // 递归查找根节点，并进行路径压缩
        if (dsu_parent_[node_id] == node_id)
        {
            return node_id; // 找到根节点
        }
        // 路径压缩：将当前节点的父节点直接指向根节点
        dsu_parent_[node_id] = find_set(dsu_parent_[node_id]);
        return dsu_parent_[node_id];
    }

    // 递归函数，用于 LCA 预计算
    // node: 当前处理的节点
    // d: 当前节点的深度
    void precompute_lca(TreeNodePtr node, int d)
    {
        if (!node || node->node_id < 0 || node->node_id >= up_.size())
            return; // 节点无效或 ID 超出范围

        node->depth = d; // 设置节点深度

        // 获取父节点指针 (通过 weak_ptr 的 lock() 获取 shared_ptr)
        TreeNodePtr parent_ptr = node->parent.lock();
        up_[node->node_id][0] = parent_ptr; // 设置节点的第 2^0 (即 1) 个祖先为其父节点

        // 计算更高层的祖先
        for (int i = 1; i < MAX_LCA_LOG; ++i)
        {
            // 节点 node 的第 2^i 个祖先 = 节点 node 的第 2^(i-1) 个祖先 的 第 2^(i-1) 个祖先
            TreeNodePtr ancestor = up_[node->node_id][i - 1]; // 获取第 2^(i-1) 个祖先
            if (ancestor && ancestor->node_id >= 0 && ancestor->node_id < up_.size() && i - 1 < up_[ancestor->node_id].size())
            {
                // 如果 2^(i-1) 祖先存在且有效，则计算 2^i 祖先
                up_[node->node_id][i] = up_[ancestor->node_id][i - 1];
            }
            else
            {
                // 如果 2^(i-1) 祖先不存在，则更高层的祖先也不存在
                up_[node->node_id][i] = nullptr;
            }
        }

        // 递归处理所有子节点
        for (const auto &child : node->children)
        {
            precompute_lca(child, d + 1); // 子节点深度加 1
        }
    }

    // 查找两个节点的最近公共祖先 (LCA)
    // u, v: 要查找 LCA 的两个节点的指针
    // 返回: LCA 节点的指针，如果不存在或输入无效则返回 nullptr
    // 逻辑：
    // 1. 将深度较深的节点提升到与深度较浅节点相同的高度。
    // 2. 如果此时两节点相同，则该节点即为 LCA。
    // 3. 否则，同时提升两个节点，每次提升 2^i 步，直到它们的父节点相同。
    // 4. 父节点即为 LCA。
    TreeNodePtr find_lca(TreeNodePtr u, TreeNodePtr v)
    {
        if (!u || !v || u->node_id < 0 || v->node_id < 0 || u->node_id >= up_.size() || v->node_id >= up_.size())
            return nullptr; // 无效输入

        // 确保 u 是深度较大的节点
        if (u->depth < v->depth)
            std::swap(u, v);

        // 1. 将 u 提升到与 v 相同的高度
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i)
        {
            // 检查索引是否有效
            if (i < up_[u->node_id].size())
            {
                TreeNodePtr ancestor = up_[u->node_id][i]; // 获取 u 的第 2^i 个祖先
                // 如果祖先存在且其深度仍不小于 v 的深度，则提升 u
                if (ancestor && ancestor->depth >= v->depth)
                {
                    u = ancestor;
                }
            }
        }

        // 2. 如果此时 u 和 v 相同，则它们就是 LCA
        if (u->node_id == v->node_id)
            return u;

        // 3. 同时提升 u 和 v，直到它们的父节点相同
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i)
        {
            // 检查索引是否有效
            if (i < up_[u->node_id].size() && i < up_[v->node_id].size())
            {
                TreeNodePtr parent_u = up_[u->node_id][i]; // u 的第 2^i 个祖先
                TreeNodePtr parent_v = up_[v->node_id][i]; // v 的第 2^i 个祖先

                // 如果两个祖先都存在且不相同，则同时提升 u 和 v
                if (parent_u && parent_v && parent_u->node_id != parent_v->node_id)
                {
                    u = parent_u;
                    v = parent_v;
                }
            }
        }

        // 4. 此时 u 和 v 的父节点（即它们的第 2^0 个祖先）就是 LCA
        if (!up_[u->node_id].empty())
        {
            return up_[u->node_id][0]; // 返回 u 的父节点
        }
        return nullptr; // 理论上在同一棵树中总能找到 LCA，除非是不同树的节点
    }

    // --- Caching Methods ---
    bool saveIndex(const std::string &filename) const
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file for writing index: " << filename << std::endl;
            return false;
        }

        file << "num_nodes " << next_node_id_ << "\n";
        file << "num_hyperedges " << hypergraph_.numHyperedges() << "\n";

        // Save node definitions
        for (int i = 0; i < next_node_id_; ++i)
        {
            const auto &node = nodes_[i];
            if (!node)
                continue;

            file << "node " << node->node_id << " "
                 << node->is_leaf << " "
                 << node->hyperedge_id << " "
                 << node->intersection_size << " "; // Save size

            auto parent_ptr = node->parent.lock();
            file << (parent_ptr ? parent_ptr->node_id : -1) << " ";

            file << node->children.size();
            for (const auto &child : node->children)
            {
                if (child)
                    file << " " << child->node_id;
            }
            file << "\n";

            // NOTE: Intersection vertices are NOT saved to keep the cache format
            // consistent between buildIndex and buildIndexSizeOnly.
            // If needed, add a flag or separate save method.
        }

        // Save roots
        file << "roots " << roots_.size();
        for (const auto &root : roots_)
        {
            if (root)
                file << " " << root->node_id;
        }
        file << "\n";

        // Save hyperedge_to_leaf mapping
        file << "hyperedge_to_leaf " << hyperedge_to_leaf_.size();
        for (const auto &leaf_ptr : hyperedge_to_leaf_)
        {
            file << " " << (leaf_ptr ? leaf_ptr->node_id : -1);
        }
        file << "\n";

        return file.good();
    }

    void findRoots()
    {
        roots_.clear();
        std::vector<bool> is_root_added(next_node_id_, false);
        for (int i = 0; i < next_node_id_; ++i)
        {
            // Check if node exists and parent is expired (or was never set)
            if (i < nodes_.size() && nodes_[i] && nodes_[i]->parent.expired())
            {
                int root_id = find_set(i); // Find the ultimate root of the DSU set
                if (root_id != -1 && root_id < is_root_added.size() && !is_root_added[root_id])
                {
                    if (root_id < nodes_.size() && nodes_[root_id])
                    { // Ensure the root node pointer is valid
                        roots_.push_back(nodes_[root_id]);
                        is_root_added[root_id] = true;
                    }
                }
            }
        }
        if (roots_.empty() && next_node_id_ > 0)
        {
            std::cerr << "Warning: No root nodes found after build. Index might be invalid." << std::endl;
        }
    }

    // --- Helper methods for building ---
    void clearIndexData()
    {
        nodes_.clear();
        roots_.clear();
        next_node_id_ = 0;
        // Resize and clear hyperedge_to_leaf_ based on current hypergraph state
        size_t num_hyperedges = hypergraph_.numHyperedges();
        if (hyperedge_to_leaf_.size() != num_hyperedges)
        {
            hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
        }
        else
        {
            std::fill(hyperedge_to_leaf_.begin(), hyperedge_to_leaf_.end(), nullptr);
        }
        dsu_parent_.clear();
        up_.clear(); // Clear LCA data as well

        // Clear neighbor data structure (修改点: 检查 k>=1, 大小为 k)
        if (max_intersection_k_ >= 1) {
            size_t num_hyperedges = hypergraph_.numHyperedges();
            edge_neighbors_by_size_.assign(num_hyperedges, std::vector<std::vector<int>>(max_intersection_k_)); // 大小为 k
            // Clear inner vectors explicitly
            for (auto& mid_vec : edge_neighbors_by_size_) {
                for (auto& inner_vec : mid_vec) {
                    inner_vec.clear();
                }
            }
       } else {
            edge_neighbors_by_size_.clear();
       }
    }

    void mergeNodesSizeOnly(const std::vector<std::tuple<int, int, int>> &merge_candidates)
    {
        for (const auto &candidate : merge_candidates)
        {
            int size = std::get<0>(candidate);
            int node1_id = std::get<1>(candidate);
            int node2_id = std::get<2>(candidate);

            int root1_id = find_set(node1_id);
            int root2_id = find_set(node2_id);

            if (root1_id != root2_id && root1_id != -1 && root2_id != -1)
            {
                TreeNodePtr root1_node = nodes_[root1_id];
                TreeNodePtr root2_node = nodes_[root2_id];
                bool merged = false;

                auto updateDsu = [&](const auto &self, TreeNodePtr node, int new_root_id) -> void
                {
                    if (node->node_id < 0 || node->node_id >= dsu_parent_.size())
                        return;
                    dsu_parent_[node->node_id] = new_root_id;
                    for (auto &child : node->children)
                    {
                        if (child)
                            self(self, child, new_root_id);
                    }
                };

                // Try merging into existing internal nodes (only check size)
                if (!root1_node->is_leaf && root1_node->intersection_size == size)
                {
                    root1_node->children.push_back(root2_node);
                    root2_node->parent = root1_node;
                    updateDsu(updateDsu, root2_node, root1_id);
                    // dsu_parent_[root2_id] = root1_id;
                    merged = true;
                }
                else if (!root2_node->is_leaf && root2_node->intersection_size == size)
                {
                    root2_node->children.push_back(root1_node);
                    root1_node->parent = root2_node;
                    updateDsu(updateDsu, root1_node, root2_id);
                    // dsu_parent_[root1_id] = root2_id;
                    merged = true;
                }

                // Create new parent if not merged
                if (!merged)
                {
                    int parent_node_id = next_node_id_++;
                    TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                    parent->is_leaf = false;
                    parent->intersection_size = size; // Only store size
                    // parent->intersection_vertices is not stored

                    parent->children.push_back(root1_node);
                    parent->children.push_back(root2_node);
                    root1_node->parent = parent;
                    root2_node->parent = parent;

                    if (parent_node_id >= nodes_.size())
                        nodes_.resize(parent_node_id + 1);
                    nodes_[parent_node_id] = parent;
                    if (parent_node_id >= dsu_parent_.size())
                        dsu_parent_.resize(parent_node_id + 1);

                    dsu_parent_[parent_node_id] = parent_node_id;
                    dsu_parent_[root1_id] = parent_node_id;
                    dsu_parent_[root2_id] = parent_node_id;
                }
            }
        }
    }

    void mergeNodes(const std::vector<std::tuple<int, std::vector<int>, int, int>> &merge_candidates)
    {
        for (const auto &candidate : merge_candidates)
        {
            int size = std::get<0>(candidate);
            const auto &vertices = std::get<1>(candidate);
            int node1_id = std::get<2>(candidate);
            int node2_id = std::get<3>(candidate);

            int root1_id = find_set(node1_id);
            int root2_id = find_set(node2_id);

            if (root1_id != root2_id && root1_id != -1 && root2_id != -1)
            {
                TreeNodePtr root1_node = nodes_[root1_id];
                TreeNodePtr root2_node = nodes_[root2_id];
                bool merged = false;

                auto updateDsu = [&](const auto &self, TreeNodePtr node, int new_root_id) -> void
                {
                    if (node->node_id < 0 || node->node_id >= dsu_parent_.size())
                        return; // Bounds check
                    dsu_parent_[node->node_id] = new_root_id;
                    for (auto &child : node->children)
                    {
                        if (child)
                            self(self, child, new_root_id);
                    }
                };

                // Try merging into existing internal nodes if size and vertices match
                if (!root1_node->is_leaf && root1_node->intersection_size == size && root1_node->intersection_vertices == vertices)
                {
                    root1_node->children.push_back(root2_node);
                    root2_node->parent = root1_node;
                    updateDsu(updateDsu, root2_node, root1_id); // Update DSU for the merged subtree
                    // dsu_parent_[root2_id] = root1_id; // updateDsu handles this
                    merged = true;
                }
                else if (!root2_node->is_leaf && root2_node->intersection_size == size && root2_node->intersection_vertices == vertices)
                {
                    root2_node->children.push_back(root1_node);
                    root1_node->parent = root2_node;
                    updateDsu(updateDsu, root1_node, root2_id); // Update DSU for the merged subtree
                    // dsu_parent_[root1_id] = root2_id; // updateDsu handles this
                    merged = true;
                }

                // If no suitable existing node, create a new parent
                if (!merged)
                {
                    int parent_node_id = next_node_id_++;
                    TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                    parent->is_leaf = false;
                    parent->intersection_size = size;
                    parent->intersection_vertices = vertices; // Store vertices
                    parent->children.push_back(root1_node);
                    parent->children.push_back(root2_node);
                    root1_node->parent = parent;
                    root2_node->parent = parent;

                    // Ensure nodes_ and dsu_parent_ are large enough
                    if (parent_node_id >= nodes_.size())
                        nodes_.resize(parent_node_id + 1);
                    nodes_[parent_node_id] = parent;
                    if (parent_node_id >= dsu_parent_.size())
                        dsu_parent_.resize(parent_node_id + 1);

                    dsu_parent_[parent_node_id] = parent_node_id;
                    dsu_parent_[root1_id] = parent_node_id;
                    dsu_parent_[root2_id] = parent_node_id;
                }
            }
        }
    }

    void sortMergeCandidatesSizeOnly(std::vector<std::tuple<int, int, int>> &merge_candidates)
    {
        std::sort(merge_candidates.begin(), merge_candidates.end(),
                  [](const auto &a, const auto &b)
                  {
                      // Sort by size descending
                      return std::get<0>(a) > std::get<0>(b);
                  });
    }

    void calculateAndPrepareMergeCandidates(
        const std::vector<TreeNodePtr> &current_leaf_nodes,
        std::vector<std::tuple<int, std::vector<int>, int, int>> &merge_candidates)
    {
        std::mutex merge_candidates_mutex;
        size_t num_leaves = current_leaf_nodes.size();
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 1;
        std::vector<std::thread> threads(num_threads);

        merge_candidates.reserve(num_leaves * (num_leaves - 1) / 2);

        auto calculate_leaf_intersections =
            [&](size_t start_idx, size_t end_idx)
        {
            std::vector<std::tuple<int, std::vector<int>, int, int>> local_candidates;
            local_candidates.reserve((end_idx - start_idx) * num_leaves / 2);

            for (size_t i = start_idx; i < end_idx; ++i)
            {
                for (size_t j = i + 1; j < num_leaves; ++j)
                {
                    auto intersection_result = calculate_intersection(current_leaf_nodes[i], current_leaf_nodes[j]);
                    int intersection_size = intersection_result.first;
                    std::vector<int> intersection_verts = std::move(intersection_result.second);

                    if (intersection_size > 0)
                    {
                        local_candidates.emplace_back(intersection_size, std::move(intersection_verts), current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                    }
                }
            }
            std::lock_guard<std::mutex> lock(merge_candidates_mutex);
            merge_candidates.insert(merge_candidates.end(),
                                    std::make_move_iterator(local_candidates.begin()),
                                    std::make_move_iterator(local_candidates.end()));
        };

        size_t chunk_size = (num_leaves + num_threads - 1) / num_threads;
        size_t current_start = 0;
        for (unsigned int t = 0; t < num_threads; ++t)
        {
            size_t current_end = std::min(current_start + chunk_size, num_leaves);
            if (current_start >= current_end)
                break;
            threads[t] = std::thread(calculate_leaf_intersections, current_start, current_end);
            current_start = current_end;
        }

        for (unsigned int t = 0; t < threads.size(); ++t)
        {
            if (threads[t].joinable())
            {
                threads[t].join();
            }
        }
    }

    void calculateAndPrepareMergeCandidatesSizeOnly(
        const std::vector<TreeNodePtr> &current_leaf_nodes,
        std::vector<std::tuple<int, int, int>> &merge_candidates)
    {
        std::mutex merge_candidates_mutex;
        size_t num_leaves = current_leaf_nodes.size();
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 1;
        std::vector<std::thread> threads(num_threads);

        merge_candidates.reserve(num_leaves * (num_leaves - 1) / 2);

        auto calculate_leaf_intersections_size_only =
            [&](size_t start_idx, size_t end_idx)
        {
            std::vector<std::tuple<int, int, int>> local_candidates;
            local_candidates.reserve((end_idx - start_idx) * num_leaves / 2);

            for (size_t i = start_idx; i < end_idx; ++i)
            {
                for (size_t j = i + 1; j < num_leaves; ++j)
                {
                    auto intersection_result = calculate_intersection(current_leaf_nodes[i], current_leaf_nodes[j]);
                    int intersection_size = intersection_result.first;

                    if (intersection_size > 0)
                    {
                        local_candidates.emplace_back(intersection_size, current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                    }
                }
            }
            std::lock_guard<std::mutex> lock(merge_candidates_mutex);
            merge_candidates.insert(merge_candidates.end(), local_candidates.begin(), local_candidates.end());
        };

        size_t chunk_size = (num_leaves + num_threads - 1) / num_threads;
        size_t current_start = 0;
        for (unsigned int t = 0; t < num_threads; ++t)
        {
            size_t current_end = std::min(current_start + chunk_size, num_leaves);
            if (current_start >= current_end)
                break;
            threads[t] = std::thread(calculate_leaf_intersections_size_only, current_start, current_end);
            current_start = current_end;
        }

        for (unsigned int t = 0; t < threads.size(); ++t)
        {
            if (threads[t].joinable())
            {
                threads[t].join();
            }
        }
    }

    void sortMergeCandidates(std::vector<std::tuple<int, std::vector<int>, int, int>> &merge_candidates)
    {
        std::sort(merge_candidates.begin(), merge_candidates.end(),
                  [](const auto &a, const auto &b)
                  {
                      if (std::get<0>(a) != std::get<0>(b))
                      {
                          return std::get<0>(a) > std::get<0>(b); // Sort by size descending
                      }
                      // Optional: Secondary sort by vertices for deterministic merging
                      return std::get<1>(a) < std::get<1>(b);
                  });
    }

    void initializeDSU()
    {
        dsu_parent_.resize(next_node_id_);
        for (int i = 0; i < next_node_id_; ++i)
        {
            dsu_parent_[i] = i;
        }
    }

    // Helper to recompute LCA for all roots (used after building or loading)
    void recomputeAllLCA()
    {
        if (next_node_id_ == 0)
            return; // Nothing to compute for empty index

        if (up_.size() < next_node_id_)
        {
            up_.resize(next_node_id_);
        }
        // Initialize LCA array entries
        for (int i = 0; i < next_node_id_; ++i)
        {
            if (i < up_.size())
            { // Check bounds before assigning
                up_[i].assign(MAX_LCA_LOG, nullptr);
            }
        }
        // Compute LCA starting from each root
        for (const auto &root : roots_)
        {
            if (root)
            {                            // Check if root pointer is valid
                precompute_lca(root, 0); // Root depth is 0
            }
        }
    }

    bool loadIndex(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false; // Indicate cache miss (file not found)
        }

        std::string line_type;
        int num_nodes = 0;
        size_t num_hyperedges_expected = 0;

        // Read header
        file >> line_type >> num_nodes;
        if (file.fail() || line_type != "num_nodes" || num_nodes <= 0)
        {
            std::cerr << "Error: Invalid cache file format (num_nodes)." << std::endl;
            return false;
        }
        file >> line_type >> num_hyperedges_expected;
        if (file.fail() || line_type != "num_hyperedges" || num_hyperedges_expected != hypergraph_.numHyperedges())
        {
            std::cerr << "Error: Cache file hyperedge count mismatch with current hypergraph." << std::endl;
            return false;
        }

        // Reset internal state before loading
        clearIndexData();                                            // Use helper to reset
        nodes_.resize(num_nodes, nullptr);                           // Resize nodes_ based on cache info
        hyperedge_to_leaf_.resize(num_hyperedges_expected, nullptr); // Resize based on cache info
        next_node_id_ = num_nodes;                                   // Set next ID correctly

        std::vector<int> parent_ids(num_nodes, -1);
        std::vector<std::vector<int>> children_ids(num_nodes);

        std::string line;
        std::getline(file, line); // Consume rest of the num_hyperedges line

        // First pass: Read node data and temporary links
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            iss >> line_type;

            if (line_type == "node")
            {
                int id, h_id, i_size, p_id, child_count;
                bool is_leaf;
                // Read node data (intersection_vertices are not read)
                iss >> id >> is_leaf >> h_id >> i_size >> p_id >> child_count;
                if (iss.fail() || id < 0 || id >= num_nodes)
                {
                    std::cerr << "Error: Invalid node data in cache." << std::endl;
                    return false;
                }

                nodes_[id] = std::make_shared<TreeNode>(id);
                nodes_[id]->is_leaf = is_leaf;
                nodes_[id]->hyperedge_id = h_id;
                nodes_[id]->intersection_size = i_size; // Store size
                parent_ids[id] = p_id;
                children_ids[id].resize(child_count);
                for (int i = 0; i < child_count; ++i)
                {
                    if (!(iss >> children_ids[id][i]))
                    {
                        std::cerr << "Error: Reading child IDs failed in cache." << std::endl;
                        return false;
                    }
                }
            }
            else if (line_type == "roots")
            {
                int count, root_id;
                iss >> count;
                if (iss.fail())
                {
                    std::cerr << "Error: Reading root count failed." << std::endl;
                    return false;
                }
                roots_.reserve(count);
                for (int i = 0; i < count; ++i)
                {
                    iss >> root_id;
                    if (iss.fail() || root_id < 0 || root_id >= num_nodes || !nodes_[root_id])
                    {
                        std::cerr << "Error: Invalid root ID in cache." << std::endl;
                        return false;
                    }
                    roots_.push_back(nodes_[root_id]);
                }
            }
            else if (line_type == "hyperedge_to_leaf")
            {
                size_t count;
                int leaf_id;
                iss >> count;
                if (iss.fail() || count != hyperedge_to_leaf_.size())
                {
                    std::cerr << "Error: hyperedge_to_leaf count mismatch in cache." << std::endl;
                    return false;
                }
                for (size_t i = 0; i < count; ++i)
                {
                    iss >> leaf_id;
                    if (iss.fail())
                    {
                        std::cerr << "Error: Reading leaf ID failed." << std::endl;
                        return false;
                    }
                    if (leaf_id >= 0 && leaf_id < num_nodes && nodes_[leaf_id])
                    {
                        hyperedge_to_leaf_[i] = nodes_[leaf_id];
                    }
                    else if (leaf_id != -1)
                    {
                        std::cerr << "Error: Invalid leaf ID in hyperedge_to_leaf map." << std::endl;
                        return false;
                    }
                }
            }
        }

        if (file.bad())
        { // Check for read errors
            std::cerr << "Error: File read error occurred during cache loading." << std::endl;
            return false;
        }

        // Second pass: Link parents and children using shared/weak pointers
        for (int i = 0; i < num_nodes; ++i)
        {
            if (!nodes_[i])
                continue; // Skip if node wasn't loaded properly (shouldn't happen with checks)
            // Link parent (weak_ptr)
            if (parent_ids[i] != -1)
            {
                if (parent_ids[i] >= 0 && parent_ids[i] < num_nodes && nodes_[parent_ids[i]])
                {
                    nodes_[i]->parent = nodes_[parent_ids[i]];
                }
                else
                {
                    std::cerr << "Error: Invalid parent link ID (" << parent_ids[i] << ") for node " << i << " in cache." << std::endl;
                    return false;
                }
            }
            // Link children (shared_ptr)
            nodes_[i]->children.reserve(children_ids[i].size());
            for (int child_id : children_ids[i])
            {
                if (child_id >= 0 && child_id < num_nodes && nodes_[child_id])
                {
                    nodes_[i]->children.push_back(nodes_[child_id]);
                }
                else
                {
                    std::cerr << "Error: Invalid child link ID (" << child_id << ") for node " << i << " in cache." << std::endl;
                    return false;
                }
            }
        }

        // Basic validation: Check if roots were loaded correctly
        if (roots_.empty() && num_nodes > 0)
        {
            std::cerr << "Error: No roots loaded from cache, but nodes exist." << std::endl;
            // Attempt to find roots manually as a fallback, but indicates cache issue
            findRoots(); // Try to recover roots
            if (roots_.empty())
                return false; // Still no roots found, load failed
        }

        // Recompute LCA after successfully loading the structure
        recomputeAllLCA();

        return true; // Successfully loaded
    }

     // Helper to precompute LCA for all roots (used after building or loading)
     void precomputeAllLCA()
     {
         if (next_node_id_ == 0)
             return; // Nothing to compute for empty index
 
         // Ensure up_ array is correctly sized
         if (up_.size() < next_node_id_)
         {
             up_.resize(next_node_id_);
         }
         // Initialize LCA array entries for all potential nodes
         for (int i = 0; i < next_node_id_; ++i)
         {
             if (i < up_.size())
             { // Check bounds before assigning
                 up_[i].assign(MAX_LCA_LOG, nullptr);
             }
             // Reset depth in case it was loaded incorrectly or needs recalculation
             if (i < nodes_.size() && nodes_[i]) {
                  nodes_[i]->depth = 0; // Reset depth before recalculation
             }
         }
         // Compute LCA starting from each root
         for (const auto &root : roots_)
         {
             if (root)
             {                            // Check if root pointer is valid
                 precompute_lca(root, 0); // Start recursive precomputation from root (depth 0)
             }
         }
     }

     
    // --- 新增：填充邻居信息 (完整版) ---
    void populateEdgeNeighbors(const std::vector<std::tuple<int, std::vector<int>, int, int>>& merge_candidates) {
        if (max_intersection_k_ < 1) return; // 不需要存储邻居信息 (修改点)

        // 清空旧的邻居信息
        for (auto& mid_vec : edge_neighbors_by_size_) {
            for (auto& inner_vec : mid_vec) {
                inner_vec.clear();
            }
        }

        for (const auto& candidate : merge_candidates) {
            int size = std::get<0>(candidate);
            int node1_id = std::get<2>(candidate);
            int node2_id = std::get<3>(candidate);

            // 只处理叶子节点之间的交集，并且交集大小在 [1, k] 范围内 (修改点: size >= 1)
            if (size >= 1 && size <= max_intersection_k_ &&
                node1_id >= 0 && node1_id < nodes_.size() && nodes_[node1_id]->is_leaf && // 增加 node_id 边界检查
                node2_id >= 0 && node2_id < nodes_.size() && nodes_[node2_id]->is_leaf)   // 增加 node_id 边界检查
            {
                int edge1_id = nodes_[node1_id]->hyperedge_id;
                int edge2_id = nodes_[node2_id]->hyperedge_id;

                // 检查边界并添加到邻居列表 (修改点: 索引 size - 1)
                if (edge1_id >= 0 && edge1_id < edge_neighbors_by_size_.size() &&
                    edge2_id >= 0 && edge2_id < edge_neighbors_by_size_.size() &&
                    (size - 1) >= 0 && (size - 1) < edge_neighbors_by_size_[edge1_id].size() &&
                    (size - 1) < edge_neighbors_by_size_[edge2_id].size())
                {
                    edge_neighbors_by_size_[edge1_id][size - 1].push_back(edge2_id);
                    edge_neighbors_by_size_[edge2_id][size - 1].push_back(edge1_id);
                }
            }
        }
        // 可选排序去重
    }

    // --- 新增：填充邻居信息 (SizeOnly 版) ---
    void populateEdgeNeighborsSizeOnly(const std::vector<std::tuple<int, int, int>>& merge_candidates_size_only) {
         if (max_intersection_k_ < 1) return; // (修改点)

         // 清空旧的邻居信息
         for (auto& mid_vec : edge_neighbors_by_size_) {
             for (auto& inner_vec : mid_vec) {
                 inner_vec.clear();
             }
         }

         for (const auto& candidate : merge_candidates_size_only) {
             int size = std::get<0>(candidate);
             int node1_id = std::get<1>(candidate);
             int node2_id = std::get<2>(candidate);

             // 只处理叶子节点之间的交集，并且交集大小在 [1, k] 范围内 (修改点: size >= 1)
             if (size >= 1 && size <= max_intersection_k_ &&
                 node1_id >= 0 && node1_id < nodes_.size() && nodes_[node1_id]->is_leaf && // 增加 node_id 边界检查
                 node2_id >= 0 && node2_id < nodes_.size() && nodes_[node2_id]->is_leaf)   // 增加 node_id 边界检查
             {
                 int edge1_id = nodes_[node1_id]->hyperedge_id;
                 int edge2_id = nodes_[node2_id]->hyperedge_id;

                 // 检查边界并添加到邻居列表 (修改点: 索引 size - 1)
                 if (edge1_id >= 0 && edge1_id < edge_neighbors_by_size_.size() &&
                     edge2_id >= 0 && edge2_id < edge_neighbors_by_size_.size() &&
                     (size - 1) >= 0 && (size - 1) < edge_neighbors_by_size_[edge1_id].size() &&
                     (size - 1) < edge_neighbors_by_size_[edge2_id].size())
                 {
                     edge_neighbors_by_size_[edge1_id][size - 1].push_back(edge2_id);
                     edge_neighbors_by_size_[edge2_id][size - 1].push_back(edge1_id);
                 }
             }
         }
         // 可选排序去重
    }


    // --- 新增：重新计算并填充邻居信息 (用于缓存加载后) ---
    void recalculateAndPopulateNeighbors() {
        if (max_intersection_k_ < 2) return;

        // 1. 获取所有有效的叶子节点
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(hyperedge_to_leaf_.size());
        for(const auto& leaf_ptr : hyperedge_to_leaf_) {
            if (leaf_ptr) { // 只考虑有效的叶子节点映射
                current_leaf_nodes.push_back(leaf_ptr);
            }
        }
        if (current_leaf_nodes.empty()) return;

        // 2. 重新计算所有叶子节点对之间的交集
        std::vector<std::tuple<int, std::vector<int>, int, int>> merge_candidates;
        calculateAndPrepareMergeCandidates(current_leaf_nodes, merge_candidates); // 重用计算逻辑

        // 3. 填充邻居信息
        populateEdgeNeighbors(merge_candidates);
    }

    // --- 新增：重新计算并填充邻居信息 (SizeOnly, 用于缓存加载后) ---
    void recalculateAndPopulateNeighborsSizeOnly() {
         if (max_intersection_k_ < 2) return;

         // 1. 获取所有有效的叶子节点 (同上)
         std::vector<TreeNodePtr> current_leaf_nodes;
         current_leaf_nodes.reserve(hyperedge_to_leaf_.size());
         for(const auto& leaf_ptr : hyperedge_to_leaf_) {
             if (leaf_ptr) {
                 current_leaf_nodes.push_back(leaf_ptr);
             }
         }
         if (current_leaf_nodes.empty()) return;

         // 2. 重新计算所有叶子节点对之间的交集大小
         std::vector<std::tuple<int, int, int>> merge_candidates_size_only;
         calculateAndPrepareMergeCandidatesSizeOnly(current_leaf_nodes, merge_candidates_size_only); // 重用计算逻辑

         // 3. 填充邻居信息
         populateEdgeNeighborsSizeOnly(merge_candidates_size_only);
    }

 
};

#endif // HYPERGRAPH_TREE_INDEX_H
