#ifndef HYPERGRAPH_TREE_INDEX_H
#define HYPERGRAPH_TREE_INDEX_H

#include "Hypergraph.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>
#include <stdexcept> // For std::out_of_range
#include <tuple>     // For std::tuple
#include <fstream> // Add this include
#include <string>  // Add this include
#include <iomanip> // Add this include for std::quoted


// 前向声明
class HypergraphTreeIndex;

// 树节点结构
struct TreeNode {
    bool is_leaf = false;
    int intersection_size = 0; // 仅内部节点使用: 代表其子树所覆盖的超边集合之间两两交集大小的最大值
    int hyperedge_id = -1;     // 仅叶子节点使用: 对应的原始超边ID
    std::vector<std::shared_ptr<TreeNode>> children; // 子节点列表
    std::weak_ptr<TreeNode> parent; // 指向父节点的弱指针，避免循环引用
    int node_id = -1; // 节点的唯一标识符, 对应其在 nodes_ 向量中的索引
    int depth = 0; // 节点在树中的深度，用于LCA计算

    // 构造函数
    TreeNode(int id) : node_id(id) {}
};

// 定义 TreeNodePtr 类型别名，方便使用
using TreeNodePtr = std::shared_ptr<TreeNode>;

// 超图树形索引类
class HypergraphTreeIndex {
public:
    // 构造函数
    // hg: 关联的 Hypergraph 对象引用
    HypergraphTreeIndex(Hypergraph& hg) : hypergraph_(hg), next_node_id_(0) {
         size_t num_hyperedges = hypergraph_.numHyperedges();
         // 预估最终节点数大约是叶子节点的两倍（叶子节点+内部节点）
         nodes_.reserve(num_hyperedges * 2);
         // 预分配LCA的up数组空间，维度为 [节点数][MAX_LCA_LOG]
         up_.reserve(num_hyperedges * 2);
         // 预分配超边ID到叶子节点的映射向量
         hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
    }


    // 构建索引树
    // 基于超边之间的交集大小，采用类似层次聚类的方法构建树
    void buildIndex() {
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0) return; // 如果超图没有边，则无需构建

        // --- 清理和准备 ---
        nodes_.clear(); // 清空旧的节点数据
        roots_.clear(); // 清空旧的根节点列表
        next_node_id_ = 0; // 重置节点ID分配器
        // 确保 hyperedge_to_leaf_ 映射向量大小正确并清空旧指针
        if (hyperedge_to_leaf_.size() < num_hyperedges) {
            hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
        } else {
            // 使用 fill 清空现有元素
            std::fill(hyperedge_to_leaf_.begin(), hyperedge_to_leaf_.end(), nullptr);
        }
        // 清空并查集数据
        dsu_parent_.clear();


        // --- 1. 初始化叶子节点 ---
        // 每个有效的超边对应一个叶子节点
        std::vector<TreeNodePtr> current_leaf_nodes; // 临时存储叶子节点，用于计算初始交集
        current_leaf_nodes.reserve(num_hyperedges);

        for (int i = 0; i < num_hyperedges; ++i) {
            // 跳过空的超边
            if (hypergraph_.getHyperedge(i).size() == 0) continue;

            // 创建叶子节点
            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++);
            leaf->is_leaf = true;
            leaf->hyperedge_id = i; // 记录对应的超边ID
            nodes_.push_back(leaf); // 将节点添加到全局节点列表

            // 建立超边ID到叶子节点的映射
            if (i < hyperedge_to_leaf_.size()) {
                 hyperedge_to_leaf_[i] = leaf;
            } else {
                 // 处理超边ID超出预分配范围的情况 (理论上不应发生，除非Hypergraph实现有变)
                 throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector");
            }
            current_leaf_nodes.push_back(leaf); // 加入临时列表
        }

        // 如果没有有效的叶子节点（所有超边都为空），则结束
        if (current_leaf_nodes.empty()) return;

        // --- 2. 计算初始合并候选 ---
        // 计算所有叶子节点对之间的交集大小
        std::vector<std::tuple<int, int, int>> merge_candidates; // 存储 <交集大小, 节点1 ID, 节点2 ID>
        merge_candidates.reserve(current_leaf_nodes.size() * (current_leaf_nodes.size() - 1) / 2); // 预估大小

        for (size_t i = 0; i < current_leaf_nodes.size(); ++i) {
            for (size_t j = i + 1; j < current_leaf_nodes.size(); ++j) {
                // 调用辅助函数计算两个叶子节点对应超边的交集
                int intersection_size = calculate_intersection_size(current_leaf_nodes[i], current_leaf_nodes[j]);
                // 只考虑交集大于0的对
                if (intersection_size > 0) {
                    merge_candidates.emplace_back(intersection_size, current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                }
            }
        }

        // 按交集大小降序排序，优先合并交集大的节点对
        std::sort(merge_candidates.rbegin(), merge_candidates.rend());

        // --- 3. 迭代合并节点 (构建树) ---
        // 使用并查集 (Disjoint Set Union - DSU) 跟踪节点的合并状态
        dsu_parent_.resize(next_node_id_); // 调整大小以容纳所有叶子节点
        // 初始化并查集：每个节点初始时指向自己 (是自己的根)
        for(int i = 0; i < next_node_id_; ++i) {
            dsu_parent_[i] = i;
        }

        // 遍历排序后的合并候选
        for (const auto& candidate : merge_candidates) {
            int size = std::get<0>(candidate); // 当前候选的交集大小
            int node1_id = std::get<1>(candidate);
            int node2_id = std::get<2>(candidate);

            // 查找两个节点当前所属集合的根节点ID
            int root1_id = find_set(node1_id);
            int root2_id = find_set(node2_id);

            // 如果两个节点不在同一个集合中 (即尚未通过其他路径合并) 且根节点有效
            if (root1_id != root2_id && root1_id != -1 && root2_id != -1) {
                // 获取根节点指针
                TreeNodePtr root1_node = nodes_[root1_id];
                TreeNodePtr root2_node = nodes_[root2_id];

                // 创建新的父节点 (内部节点)
                int parent_node_id = next_node_id_++; // 分配新ID
                TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                parent->is_leaf = false;
                parent->intersection_size = size; // 记录合并时的交集大小
                parent->children.push_back(root1_node); // 添加子节点
                parent->children.push_back(root2_node);

                // 设置子节点的父指针
                root1_node->parent = parent;
                root2_node->parent = parent;

                // 确保 nodes_ 向量有足够空间存储新父节点
                if (parent_node_id >= nodes_.size()) {
                    nodes_.resize(parent_node_id + 1);
                }
                nodes_[parent_node_id] = parent; // 将新父节点加入全局列表

                // 确保 dsu_parent_ 向量有足够空间，并初始化新父节点的父指针
                if (parent_node_id >= dsu_parent_.size()) {
                    dsu_parent_.resize(parent_node_id + 1);
                }
                dsu_parent_[parent_node_id] = parent_node_id; // 新节点初始指向自己

                // 更新并查集：将两个旧根节点的父指针指向新的父节点ID，完成合并
                dsu_parent_[root1_id] = parent_node_id;
                dsu_parent_[root2_id] = parent_node_id;
            }
             // 可选的提前退出条件: 如果所有节点都合并到了一个集合中，可以停止
             // int root_count = 0;
             // for(int i = 0; i < next_node_id_; ++i) {
             //     if (i < dsu_parent_.size() && dsu_parent_[i] == i) root_count++;
             // }
             // if (root_count == 1 && next_node_id_ > 1) break; // 确保不是只有一个叶子节点的情况
        }

        // --- 4. 确定根节点 ---
        // 合并完成后，没有父节点的节点即为树（或森林）的根节点
        roots_.clear();
        std::vector<bool> is_root_added(next_node_id_, false); // 标记已添加的根，防止重复
         for(int i = 0; i < next_node_id_; ++i) {
             // 检查节点是否存在且其父指针是否为空 (weak_ptr.expired())
             if (i < nodes_.size() && nodes_[i] && nodes_[i]->parent.expired()) {
                 int root_id = find_set(i); // 找到该节点所属集合的最终根ID
                 // 检查根ID是否有效且尚未添加
                 if (root_id != -1 && root_id < is_root_added.size() && !is_root_added[root_id]) {
                    // 检查根节点指针是否有效
                    if(root_id < nodes_.size() && nodes_[root_id]) {
                        roots_.push_back(nodes_[root_id]); // 添加到根节点列表
                        is_root_added[root_id] = true;      // 标记为已添加
                    }
                 }
             }
         }

        // --- 5. 预计算 LCA 信息 ---
        // 为后续高效查询最近公共祖先 (LCA) 做准备
        // 调整 up_ 数组大小并初始化
        if (up_.size() < next_node_id_) {
             up_.resize(next_node_id_);
        }
        // 使用 assign 清空并设置内部 vector 大小
        for(int i = 0; i < next_node_id_; ++i) {
             up_[i].assign(MAX_LCA_LOG, nullptr);
        }
        // 对每个根节点启动 DFS 进行预计算
        for(const auto& root : roots_) {
            precompute_lca(root, 0); // 计算深度和祖先信息
        }
    }

    // 查询可达性
    // u, v: 起点和终点顶点 ID
    // k: 路径上相邻超边所需的最小交集大小约束
    // 返回值: 如果存在一条从 u 到 v 的路径，路径上每一步（相邻超边）的交集都 >= k，则返回 true
    bool query(int u, int v, int k) {
        if (u == v) return true; // 同一个顶点总是可达

        // 获取顶点 u 和 v 各自关联的超边列表
        const auto& edges_u = hypergraph_.getIncidentHyperedges(u);
        const auto& edges_v = hypergraph_.getIncidentHyperedges(v);

        // 如果任一顶点不属于任何超边，则不可达
        if (edges_u.empty() || edges_v.empty()) {
            return false;
        }

        // --- 优化：检查 u 和 v 是否共享同一个超边 ---
        // 如果共享超边，则它们之间直接可达（视为交集无限大）
        int max_edge_id = hypergraph_.numHyperedges();
        // 使用 vector<bool> 作为标记，比 set/map 更快
        std::vector<bool> v_edge_flags(max_edge_id, false);
        for(int edge_id_v : edges_v) {
            // 检查边ID有效性
            if (edge_id_v >= 0 && edge_id_v < max_edge_id) {
                v_edge_flags[edge_id_v] = true; // 标记顶点v所在的超边
            }
        }
        bool common_edge_found = false;
        for(int edge_id_u : edges_u) {
            // 检查边ID有效性并查看是否被标记
            if (edge_id_u >= 0 && edge_id_u < max_edge_id && v_edge_flags[edge_id_u]) {
                common_edge_found = true;
                break; // 找到一个即可
            }
        }
        if (common_edge_found) {
             return true; // 直接可达
        }


        // --- 遍历超边对，查找满足条件的 LCA ---
        // 遍历所有可能的超边对 (edge_u, edge_v)，其中 edge_u 包含 u，edge_v 包含 v
        for (int edge_id_u : edges_u) {
            // 检查超边ID有效性及是否存在对应的叶子节点
            if (edge_id_u < 0 || edge_id_u >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_u]) continue;
            TreeNodePtr leaf_u = hyperedge_to_leaf_[edge_id_u]; // 获取 u 所在超边对应的叶子节点

            for (int edge_id_v : edges_v) {
                 // 检查超边ID有效性及是否存在对应的叶子节点
                 if (edge_id_v < 0 || edge_id_v >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_v]) continue;
                TreeNodePtr leaf_v = hyperedge_to_leaf_[edge_id_v]; // 获取 v 所在超边对应的叶子节点

                // 查找这两个叶子节点的最近公共祖先 (LCA)
                TreeNodePtr lca_node = find_lca(leaf_u, leaf_v);

                // 如果找到了 LCA，并且它是一个内部节点（非叶子），
                // 并且该 LCA 节点记录的交集大小满足约束 k
                if (lca_node && !lca_node->is_leaf && lca_node->intersection_size >= k) {
                    // 这意味着存在一条从 leaf_u 到 leaf_v 的路径，
                    // 其“瓶颈”（最小交集）由 LCA 节点的 intersection_size 表示，且满足约束
                    return true; // 找到满足条件的路径，返回可达
                }
            }
        }

        // 如果遍历完所有超边对都没有找到满足条件的 LCA，则认为不可达
        return false;
    }
        // 新增：将索引树保存到 DOT 文件
        void saveToFile(const std::string& filename) const;

private:
    Hypergraph& hypergraph_; // 关联的超图对象
    std::vector<TreeNodePtr> nodes_; // 存储树中所有节点 (叶子和内部) 的指针，node_id 是索引
    std::vector<TreeNodePtr> roots_; // 存储树/森林的根节点指针
    std::vector<TreeNodePtr> hyperedge_to_leaf_; // 超边ID到对应叶子节点的映射 (vector实现)
    int next_node_id_; // 用于分配新节点ID的计数器

    // 并查集 (DSU) 数据结构，用于构建树过程中的集合合并
    std::vector<int> dsu_parent_; // 存储每个节点的父节点ID (在DSU意义下)

    // LCA (最近公共祖先) 预计算相关
    static const int MAX_LCA_LOG = 20; // 预设的树最大深度的对数，用于Binary Lifting。可根据实际情况调整。
    std::vector<std::vector<TreeNodePtr>> up_; // up_[node_id][i] 存储 node_id 向上跳 2^i 步的祖先节点指针


    // --- 私有辅助函数 ---

    // 计算两个节点（叶子）代表的超边之间的交集大小
    // n1, n2: 两个 TreeNode 指针 (在此函数中预期是叶子节点)
    // 返回值: 两个叶子节点对应超边的交集大小
    // 注意：此实现假设调用时传入的是叶子节点。
    int calculate_intersection_size(TreeNodePtr n1, TreeNodePtr n2) {
         // 确保传入的是有效的叶子节点
         if (!n1 || !n2 || !n1->is_leaf || !n2->is_leaf) {
             // 或者可以抛出异常，取决于错误处理策略
             return 0;
         }
         // 直接调用 Hypergraph 类提供的交集计算函数
         return hypergraph_.getHyperedgeIntersection(n1->hyperedge_id, n2->hyperedge_id).size();

         /* // 旧的、更通用的实现，可以处理内部节点，但效率较低且在此处非必需
         std::vector<int> edges1, edges2;
         collect_leaf_edges(n1, edges1);
         collect_leaf_edges(n2, edges2);
         int max_intersection = 0;
         for(int e1 : edges1) {
             for (int e2 : edges2) {
                 if (e1 == e2) return std::numeric_limits<int>::max();
                 max_intersection = std::max(max_intersection, (int)hypergraph_.getHyperedgeIntersection(e1, e2).size());
             }
         }
         return max_intersection;
         */
    }

    // (此函数在当前简化版 calculate_intersection_size 中不再需要，但保留以供参考)
    // 递归收集以 node 为根的子树下的所有叶子节点对应的超边ID
    void collect_leaf_edges(TreeNodePtr node, std::vector<int>& edges) {
        if (!node) return;
        if (node->is_leaf) {
            edges.push_back(node->hyperedge_id);
        } else {
            for (const auto& child : node->children) {
                collect_leaf_edges(child, edges);
            }
        }
    }


    // 并查集查找操作（带路径压缩）
    // node_id: 要查找的节点的 ID
    // 返回值: node_id 所属集合的根节点 ID；如果 node_id 无效则返回 -1
    int find_set(int node_id) {
        // 检查 node_id 是否在有效范围内
        if (node_id < 0 || node_id >= dsu_parent_.size()) {
             return -1; // 无效 ID
        }
        // 如果节点的父节点是它自己，则它就是根
        if (dsu_parent_[node_id] == node_id) {
            return node_id;
        }
        // 路径压缩：递归查找根节点，并将路径上所有节点的父直接指向根
        dsu_parent_[node_id] = find_set(dsu_parent_[node_id]);
        return dsu_parent_[node_id];
    }


    // DFS 预计算 LCA 所需信息 (深度和祖先指针)
    // node: 当前进行 DFS 的节点
    // d: 当前节点的深度
    void precompute_lca(TreeNodePtr node, int d) {
        // 检查节点和 ID 的有效性
        if (!node || node->node_id < 0 || node->node_id >= up_.size()) return;

        node->depth = d; // 设置节点深度

        // 获取父节点指针 (使用 lock() 从 weak_ptr 获取 shared_ptr)
        TreeNodePtr parent_ptr = node->parent.lock();
        // 设置 up_[node_id][0] 为直接父节点
        up_[node->node_id][0] = parent_ptr;

        // 计算其他祖先节点 (Binary Lifting)
        for (int i = 1; i < MAX_LCA_LOG; ++i) {
            // 获取上一级祖先 (向上跳 2^(i-1) 步)
            TreeNodePtr ancestor = up_[node->node_id][i-1];
            // 如果上一级祖先存在且有效
            if (ancestor && ancestor->node_id >= 0 && ancestor->node_id < up_.size() && i-1 < up_[ancestor->node_id].size()) {
                 // 当前节点的第 i 级祖先是上一级祖先的第 (i-1) 级祖先
                 up_[node->node_id][i] = up_[ancestor->node_id][i-1];
            } else {
                 // 如果上一级祖先不存在或无效，则更高级的祖先也不存在
                 up_[node->node_id][i] = nullptr;
            }
        }

        // 递归处理所有子节点
        for (const auto& child : node->children) {
            precompute_lca(child, d + 1); // 子节点深度加 1
        }
    }

    // 使用 Binary Lifting 查找两个节点的最近公共祖先 (LCA)
    // u, v: 要查找 LCA 的两个 TreeNode 指针
    // 返回值: u 和 v 的 LCA 节点指针；如果不存在或输入无效则返回 nullptr
    TreeNodePtr find_lca(TreeNodePtr u, TreeNodePtr v) {
        // 检查输入节点和 ID 的有效性
        if (!u || !v || u->node_id < 0 || v->node_id < 0 || u->node_id >= up_.size() || v->node_id >= up_.size()) return nullptr;

        // 确保 u 是深度较大（或相等）的节点
        if (u->depth < v->depth) std::swap(u, v);

        // --- 步骤 1: 将较深的节点 u 提升到与 v 相同的深度 ---
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i) {
            // 检查索引 i 是否有效
            if (i < up_[u->node_id].size()) {
                 TreeNodePtr ancestor = up_[u->node_id][i]; // 获取 u 向上跳 2^i 步的祖先
                 // 如果祖先存在，并且其深度仍大于或等于 v 的深度
                 if (ancestor && ancestor->depth >= v->depth) {
                     u = ancestor; // 将 u 移动到该祖先位置
                 }
            }
        }

        // 如果此时 u 和 v 是同一个节点，说明 v 就是 u 的祖先（或它们是同一个节点），它就是 LCA
        if (u->node_id == v->node_id) return u;

        // --- 步骤 2: 同时提升 u 和 v，直到它们的父节点相同 ---
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i) {
             // 检查索引 i 对于 u 和 v 的 up_ 数组是否都有效
             if (i < up_[u->node_id].size() && i < up_[v->node_id].size()) {
                 TreeNodePtr parent_u = up_[u->node_id][i]; // u 的第 i 级祖先
                 TreeNodePtr parent_v = up_[v->node_id][i]; // v 的第 i 级祖先

                 // 如果两个祖先都存在，并且它们不是同一个节点
                 if (parent_u && parent_v && parent_u->node_id != parent_v->node_id) {
                     // 同时将 u 和 v 提升到这两个祖先的位置
                     u = parent_u;
                     v = parent_v;
                 }
             }
        }

        // 此时，u 和 v 的直接父节点就是它们的 LCA
        // 返回 u (或 v) 的直接父节点 (up_[...][0])
        if (!up_[u->node_id].empty()) {
            return up_[u->node_id][0];
        }
        return nullptr; // 如果 u 是根节点，没有父节点
    }
    void saveNodeToFile(std::ofstream& file, TreeNodePtr node) const;
};


// 实现 saveToFile 方法
void HypergraphTreeIndex::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    file << "graph HypergraphTree {" << std::endl;
    file << "  node [shape=record];" << std::endl; // Use record shape for more info

    // 写入所有节点定义
    for (const auto& node : nodes_) {
        if (!node) continue; // Skip nullptrs if any
         file << "  node" << node->node_id << " [label=\"{ID: " << node->node_id;
        if (node->is_leaf) {
            file << " | Type: Leaf | Hyperedge: " << node->hyperedge_id;
        } else {
            file << " | Type: Internal | Intersection: " << node->intersection_size;
        }
        file << "}\"];" << std::endl;
    }

     // 写入所有边的定义 (从根节点开始递归)
     std::vector<bool> visited_edge(next_node_id_, false); // 防止重复写入边
     for (const auto& root : roots_) {
         saveNodeToFile(file, root);
     }


    file << "}" << std::endl;
    file.close();
}

// 实现递归辅助函数 saveNodeToFile
void HypergraphTreeIndex::saveNodeToFile(std::ofstream& file, TreeNodePtr node) const {
    if (!node) return;

    for (const auto& child : node->children) {
        if (child) {
             // 写入边：父节点 -- 子节点
             file << "  node" << node->node_id << " -- node" << child->node_id << ";" << std::endl;
             // 递归访问子节点
             saveNodeToFile(file, child);
        }
    }
}


#endif // HYPERGRAPH_TREE_INDEX_H