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

// 前向声明 HypergraphTreeIndex 类
class HypergraphTreeIndex;

// 树节点结构定义
struct TreeNode {
    bool is_leaf = false;                   // 标记是否为叶子节点
    int intersection_size = 0;              // 内部节点：触发此节点创建的交集大小 (瓶颈)
    std::vector<int> intersection_vertices; // 内部节点：触发此节点创建的交集顶点列表
    int hyperedge_id = -1;                  // 叶子节点：对应的原始超边ID
    std::vector<std::shared_ptr<TreeNode>> children; // 子节点列表 (强引用)
    std::weak_ptr<TreeNode> parent;         // 指向父节点的弱指针，避免循环引用
    int node_id = -1;                       // 节点的唯一标识符, 对应其在 nodes_ 向量中的索引
    int depth = 0;                          // 节点在树中的深度，用于LCA计算

    // 构造函数，初始化节点ID
    TreeNode(int id) : node_id(id) {}
};

// 定义 TreeNodePtr 类型别名，方便使用共享指针管理 TreeNode
using TreeNodePtr = std::shared_ptr<TreeNode>;

// 超图树形索引类
// 通过构建一个层次化的树结构来加速超图可达性查询。
// 叶子节点代表原始超边，内部节点代表超边之间的交集。
class HypergraphTreeIndex {
public:
    // 构造函数
    // hg: 关联的超图对象引用
    HypergraphTreeIndex(Hypergraph& hg) : hypergraph_(hg), next_node_id_(0) {
        size_t num_hyperedges = hypergraph_.numHyperedges();
        // 预分配内存以提高效率
        nodes_.reserve(num_hyperedges * 2); // 预估节点数大约是超边数的两倍
        up_.reserve(num_hyperedges * 2);    // LCA 预计算数组
        hyperedge_to_leaf_.resize(num_hyperedges, nullptr); // 映射超边ID到叶子节点
    }

    // 构建索引树
    // 核心逻辑：自底向上构建聚类树。
    // 1. 为每个超边创建叶子节点。
    // 2. 计算所有叶子节点对之间的交集大小。
    // 3. 按交集大小降序排序，使用并查集（DSU）进行合并。
    // 4. 合并时创建新的内部父节点，记录交集信息。
    // 5. 最终形成一个或多个树（森林）。
    // 6. 对每个树进行 LCA（最近公共祖先）预计算。
    void buildIndex() {
        int num_hyperedges = hypergraph_.numHyperedges();
        if (num_hyperedges == 0) return; // 空超图直接返回

        // 清理旧数据
        nodes_.clear();
        roots_.clear();
        next_node_id_ = 0;
        // 重置超边到叶节点的映射
        if (hyperedge_to_leaf_.size() < num_hyperedges) {
            hyperedge_to_leaf_.resize(num_hyperedges, nullptr);
        } else {
            std::fill(hyperedge_to_leaf_.begin(), hyperedge_to_leaf_.end(), nullptr);
        }
        dsu_parent_.clear();

        // 1. 创建叶子节点
        std::vector<TreeNodePtr> current_leaf_nodes;
        current_leaf_nodes.reserve(num_hyperedges);
        for (int i = 0; i < num_hyperedges; ++i) {
            if (hypergraph_.getHyperedge(i).size() == 0) continue; // 跳过空超边

            TreeNodePtr leaf = std::make_shared<TreeNode>(next_node_id_++); // 创建新节点
            leaf->is_leaf = true;
            leaf->hyperedge_id = i;
            nodes_.push_back(leaf); // 加入节点列表

            // 建立超边ID到叶节点的映射
            if (i < hyperedge_to_leaf_.size()) {
                hyperedge_to_leaf_[i] = leaf;
            } else {
                // 如果 hyperedge_id 超出预分配范围，抛出异常
                throw std::out_of_range("Hyperedge ID out of range for hyperedge_to_leaf_ vector");
            }
            current_leaf_nodes.push_back(leaf); // 加入当前叶子节点列表
        }

        if (current_leaf_nodes.empty()) return; // 没有有效超边

        // 2. 计算所有叶子节点对之间的交集
        std::vector<std::tuple<int, std::vector<int>, int, int>> merge_candidates; // (交集大小, 交集顶点, 节点1ID, 节点2ID)
        merge_candidates.reserve(current_leaf_nodes.size() * (current_leaf_nodes.size() - 1) / 2); // 预估候选对数量
        for (size_t i = 0; i < current_leaf_nodes.size(); ++i) {
            for (size_t j = i + 1; j < current_leaf_nodes.size(); ++j) {
                auto intersection_result = calculate_intersection(current_leaf_nodes[i], current_leaf_nodes[j]);
                int intersection_size = intersection_result.first;
                const auto& intersection_verts = intersection_result.second;

                if (intersection_size > 0) { // 只考虑有交集的对
                    merge_candidates.emplace_back(intersection_size, intersection_verts, current_leaf_nodes[i]->node_id, current_leaf_nodes[j]->node_id);
                }
            }
        }

        // 3. 按交集大小降序排序合并候选
        std::sort(merge_candidates.rbegin(), merge_candidates.rend(),
                  [](const auto& a, const auto& b) {
                      // 按交集大小降序排列
                      return std::get<0>(a) < std::get<0>(b);
                  });

        // 初始化并查集，每个节点初始时自成一个集合
        dsu_parent_.resize(next_node_id_);
        for (int i = 0; i < next_node_id_; ++i) {
            dsu_parent_[i] = i;
        }

        // 4. 使用并查集合并节点，创建内部节点
        for (const auto& candidate : merge_candidates) {
            int size = std::get<0>(candidate);             // 交集大小
            const auto& vertices = std::get<1>(candidate); // 交集顶点
            int node1_id = std::get<2>(candidate);         // 节点1 ID
            int node2_id = std::get<3>(candidate);         // 节点2 ID

            int root1_id = find_set(node1_id); // 查找节点1所在集合的根
            int root2_id = find_set(node2_id); // 查找节点2所在集合的根

            // 如果两个节点不在同一个集合中，则合并它们
            if (root1_id != root2_id && root1_id != -1 && root2_id != -1) {
                TreeNodePtr root1_node = nodes_[root1_id]; // 获取根节点1的指针
                TreeNodePtr root2_node = nodes_[root2_id]; // 获取根节点2的指针

                // 创建新的父节点（内部节点）
                int parent_node_id = next_node_id_++;
                TreeNodePtr parent = std::make_shared<TreeNode>(parent_node_id);
                parent->is_leaf = false;
                parent->intersection_size = size;       // 记录交集大小作为瓶颈
                parent->intersection_vertices = vertices; // 记录交集顶点
                parent->children.push_back(root1_node); // 添加子节点
                parent->children.push_back(root2_node);

                // 设置子节点的父节点指针（弱引用）
                root1_node->parent = parent;
                root2_node->parent = parent;

                // 将新父节点加入节点列表
                if (parent_node_id >= nodes_.size()) {
                    nodes_.resize(parent_node_id + 1); // 动态扩展节点列表
                }
                nodes_[parent_node_id] = parent;

                // 更新并查集：将两个子树的根指向新的父节点
                if (parent_node_id >= dsu_parent_.size()) {
                    dsu_parent_.resize(parent_node_id + 1); // 动态扩展并查集数组
                }
                dsu_parent_[parent_node_id] = parent_node_id; // 新父节点指向自己
                dsu_parent_[root1_id] = parent_node_id;       // 子树根1指向新父节点
                dsu_parent_[root2_id] = parent_node_id;       // 子树根2指向新父节点
            }
        }

        // 5. 确定所有树的根节点
        roots_.clear();
        std::vector<bool> is_root_added(next_node_id_, false); // 标记根节点是否已添加
        for (int i = 0; i < next_node_id_; ++i) {
            // 如果节点存在且其父指针为空（说明是根节点）
            if (i < nodes_.size() && nodes_[i] && nodes_[i]->parent.expired()) {
                int root_id = find_set(i); // 找到该树最终的根节点ID
                // 如果根节点有效且未被添加过
                if (root_id != -1 && root_id < is_root_added.size() && !is_root_added[root_id]) {
                    if (root_id < nodes_.size() && nodes_[root_id]) { // 再次确认根节点指针有效
                        roots_.push_back(nodes_[root_id]); // 添加到根节点列表
                        is_root_added[root_id] = true;     // 标记已添加
                    }
                }
            }
        }

        // 6. 对每棵树进行 LCA 预计算
        if (up_.size() < next_node_id_) {
            up_.resize(next_node_id_); // 调整 LCA 数组大小
        }
        for (int i = 0; i < next_node_id_; ++i) {
            // 初始化 LCA 数组，每个节点的祖先指针置空
            up_[i].assign(MAX_LCA_LOG, nullptr);
        }
        // 从每个根节点开始，递归计算深度和祖先信息
        for (const auto& root : roots_) {
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
    bool query(int u, int v, int k) {
        if (u == v) return true; // 同一个顶点直接可达

        // 获取顶点关联的超边 ID 列表
        const auto& edges_u = hypergraph_.getIncidentHyperedges(u);
        const auto& edges_v = hypergraph_.getIncidentHyperedges(v);

        if (edges_u.empty() || edges_v.empty()) {
            return false; // 如果任一顶点不属于任何超边，则不可达
        }

        // 检查是否共享超边（优化：避免后续复杂的 LCA 查询）
        int max_edge_id = hypergraph_.numHyperedges();
        std::vector<bool> v_edge_flags(max_edge_id, false); // 使用布尔数组快速查找
        for (int edge_id_v : edges_v) {
            if (edge_id_v >= 0 && edge_id_v < max_edge_id) {
                v_edge_flags[edge_id_v] = true;
            }
        }
        bool common_edge_found = false;
        for (int edge_id_u : edges_u) {
            if (edge_id_u >= 0 && edge_id_u < max_edge_id && v_edge_flags[edge_id_u]) {
                common_edge_found = true; // 找到公共超边
                break;
            }
        }
        if (common_edge_found) {
            return true; // 直接可达
        }

        // 遍历所有超边对，查找 LCA
        for (int edge_id_u : edges_u) {
            // 获取超边 u 对应的叶子节点
            if (edge_id_u < 0 || edge_id_u >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_u]) continue; // 跳过无效或不存在的叶节点
            TreeNodePtr leaf_u = hyperedge_to_leaf_[edge_id_u];

            for (int edge_id_v : edges_v) {
                // 获取超边 v 对应的叶子节点
                if (edge_id_v < 0 || edge_id_v >= hyperedge_to_leaf_.size() || !hyperedge_to_leaf_[edge_id_v]) continue; // 跳过无效或不存在的叶节点
                TreeNodePtr leaf_v = hyperedge_to_leaf_[edge_id_v];

                // 查找两个叶子节点的 LCA
                TreeNodePtr lca_node = find_lca(leaf_u, leaf_v);

                // 检查 LCA 是否满足条件
                // lca_node 必须存在，必须是内部节点（代表交集），且交集大小满足阈值 k
                if (lca_node && !lca_node->is_leaf && lca_node->intersection_size >= k) {
                    return true; // 找到满足条件的路径
                }
            }
        }
        return false; // 遍历完所有组合仍未找到满足条件的路径
    }

    // 将索引树保存为 DOT 文件，用于可视化
    void saveToFile(const std::string& filename) const;

    // 估算索引占用的内存（MB）
    // 累加各个成员变量（vector, shared_ptr 等）及其内容的估算大小。
    // 使用 capacity() 而非 size() 来估算 vector 占用的内存，更接近实际情况。
    double getMemoryUsageMB() const {
        size_t memory_bytes = 0;
        memory_bytes += sizeof(*this); // 索引对象自身的大小

        // nodes_ 向量本身 + 指针 + TreeNode 对象 + 内部 vector 数据
        memory_bytes += sizeof(nodes_);
        memory_bytes += nodes_.capacity() * sizeof(TreeNodePtr); // 存储指针的开销
        for (const auto& node_ptr : nodes_) {
            if (node_ptr) {
                memory_bytes += sizeof(TreeNode); // TreeNode 对象本身
                memory_bytes += node_ptr->intersection_vertices.capacity() * sizeof(int); // intersection_vertices 数据
                memory_bytes += node_ptr->children.capacity() * sizeof(TreeNodePtr); // children 向量中的指针
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
        memory_bytes += sizeof(up_); // 外层 vector 对象
        memory_bytes += up_.capacity() * sizeof(std::vector<TreeNodePtr>); // 外层 vector 存储内层 vector 对象的开销
        for (const auto& vec : up_) {
            memory_bytes += sizeof(vec); // 内层 vector 对象本身
            memory_bytes += vec.capacity() * sizeof(TreeNodePtr); // 内层 vector 存储指针的开销
        }

        return static_cast<double>(memory_bytes) / (1024.0 * 1024.0); // 转换为 MB
    }

    // 获取索引树中的总节点数
    size_t getTotalNodes() const {
        // next_node_id_ 记录了创建的总节点数
        return next_node_id_;
    }

private:
    Hypergraph& hypergraph_;                     // 关联的超图引用
    std::vector<TreeNodePtr> nodes_;             // 存储树中所有节点的列表 (叶子和内部节点)
    std::vector<TreeNodePtr> roots_;             // 存储森林中所有树的根节点
    std::vector<TreeNodePtr> hyperedge_to_leaf_; // 映射：超边 ID -> 对应的叶子节点指针
    int next_node_id_;                           // 用于分配新节点 ID 的计数器
    std::vector<int> dsu_parent_;                // 并查集数组，用于构建树
    static const int MAX_LCA_LOG = 20;           // LCA 预计算的最大深度（2^20 足够处理大量节点）
    std::vector<std::vector<TreeNodePtr>> up_;   // LCA 预计算数组, up_[i][j] 表示节点 i 的第 2^j 个祖先

    // 计算两个叶子节点对应超边的交集
    // n1, n2: 两个叶子节点的指针
    // 返回: pair<交集大小, 交集顶点列表>
    std::pair<int, std::vector<int>> calculate_intersection(TreeNodePtr n1, TreeNodePtr n2) {
        // 确保输入是有效的叶子节点
        if (!n1 || !n2 || !n1->is_leaf || !n2->is_leaf) {
            return {0, {}}; // 无效输入返回空交集
        }
        // 调用 Hypergraph 类的方法计算交集
        std::vector<int> intersection_verts = hypergraph_.getHyperedgeIntersection(n1->hyperedge_id, n2->hyperedge_id);
        return {static_cast<int>(intersection_verts.size()), intersection_verts};
    }

    // 并查集查找操作（带路径压缩）
    // node_id: 要查找的节点 ID
    // 返回: 节点所在集合的根节点 ID，如果 node_id 无效则返回 -1
    int find_set(int node_id) {
        if (node_id < 0 || node_id >= dsu_parent_.size()) {
            return -1; // 无效 ID
        }
        // 递归查找根节点，并进行路径压缩
        if (dsu_parent_[node_id] == node_id) {
            return node_id; // 找到根节点
        }
        // 路径压缩：将当前节点的父节点直接指向根节点
        dsu_parent_[node_id] = find_set(dsu_parent_[node_id]);
        return dsu_parent_[node_id];
    }

    // 递归函数，用于 LCA 预计算
    // node: 当前处理的节点
    // d: 当前节点的深度
    void precompute_lca(TreeNodePtr node, int d) {
        if (!node || node->node_id < 0 || node->node_id >= up_.size()) return; // 节点无效或 ID 超出范围

        node->depth = d; // 设置节点深度

        // 获取父节点指针 (通过 weak_ptr 的 lock() 获取 shared_ptr)
        TreeNodePtr parent_ptr = node->parent.lock();
        up_[node->node_id][0] = parent_ptr; // 设置节点的第 2^0 (即 1) 个祖先为其父节点

        // 计算更高层的祖先
        for (int i = 1; i < MAX_LCA_LOG; ++i) {
            // 节点 node 的第 2^i 个祖先 = 节点 node 的第 2^(i-1) 个祖先 的 第 2^(i-1) 个祖先
            TreeNodePtr ancestor = up_[node->node_id][i - 1]; // 获取第 2^(i-1) 个祖先
            if (ancestor && ancestor->node_id >= 0 && ancestor->node_id < up_.size() && i - 1 < up_[ancestor->node_id].size()) {
                // 如果 2^(i-1) 祖先存在且有效，则计算 2^i 祖先
                up_[node->node_id][i] = up_[ancestor->node_id][i - 1];
            } else {
                // 如果 2^(i-1) 祖先不存在，则更高层的祖先也不存在
                up_[node->node_id][i] = nullptr;
            }
        }

        // 递归处理所有子节点
        for (const auto& child : node->children) {
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
    TreeNodePtr find_lca(TreeNodePtr u, TreeNodePtr v) {
        if (!u || !v || u->node_id < 0 || v->node_id < 0 || u->node_id >= up_.size() || v->node_id >= up_.size()) return nullptr; // 无效输入

        // 确保 u 是深度较大的节点
        if (u->depth < v->depth) std::swap(u, v);

        // 1. 将 u 提升到与 v 相同的高度
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i) {
            // 检查索引是否有效
            if (i < up_[u->node_id].size()) {
                TreeNodePtr ancestor = up_[u->node_id][i]; // 获取 u 的第 2^i 个祖先
                // 如果祖先存在且其深度仍不小于 v 的深度，则提升 u
                if (ancestor && ancestor->depth >= v->depth) {
                    u = ancestor;
                }
            }
        }

        // 2. 如果此时 u 和 v 相同，则它们就是 LCA
        if (u->node_id == v->node_id) return u;

        // 3. 同时提升 u 和 v，直到它们的父节点相同
        for (int i = MAX_LCA_LOG - 1; i >= 0; --i) {
            // 检查索引是否有效
            if (i < up_[u->node_id].size() && i < up_[v->node_id].size()) {
                TreeNodePtr parent_u = up_[u->node_id][i]; // u 的第 2^i 个祖先
                TreeNodePtr parent_v = up_[v->node_id][i]; // v 的第 2^i 个祖先

                // 如果两个祖先都存在且不相同，则同时提升 u 和 v
                if (parent_u && parent_v && parent_u->node_id != parent_v->node_id) {
                    u = parent_u;
                    v = parent_v;
                }
            }
        }

        // 4. 此时 u 和 v 的父节点（即它们的第 2^0 个祖先）就是 LCA
        if (!up_[u->node_id].empty()) {
            return up_[u->node_id][0]; // 返回 u 的父节点
        }
        return nullptr; // 理论上在同一棵树中总能找到 LCA，除非是不同树的节点
    }

    // 递归辅助函数，用于将节点及其子树结构写入 DOT 文件
    void saveNodeToFile(std::ofstream& file, TreeNodePtr node) const;
};

// 实现 saveToFile 方法
void HypergraphTreeIndex::saveToFile(const std::string& filename) const {
    std::ofstream file(filename); // 打开文件
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    file << "graph HypergraphTree {" << std::endl; // DOT 文件头
    file << "  node [shape=record];" << std::endl; // 设置节点形状

    // 遍历所有节点，定义它们的标签
    for (const auto& node : nodes_) {
        if (!node) continue; // 跳过空指针
        std::stringstream label_ss; // 使用 stringstream 构建标签字符串
        label_ss << "{ID: " << node->node_id; // 节点 ID
        if (node->is_leaf) {
            // 叶子节点标签
            label_ss << " | Type: Leaf | Hyperedge: " << node->hyperedge_id;
        } else {
            // 内部节点标签
            label_ss << " | Type: Internal | Intersection Size: " << node->intersection_size;
            // 显示部分交集顶点（如果数量不多）
            if (node->intersection_vertices.size() < 10) {
                label_ss << " | Vertices: {";
                for (size_t i = 0; i < node->intersection_vertices.size(); ++i) {
                    label_ss << node->intersection_vertices[i] << (i == node->intersection_vertices.size() - 1 ? "" : ",");
                }
                label_ss << "}";
            } else {
                label_ss << " | Vertices: ... (" << node->intersection_vertices.size() << ")";
            }
        }
        label_ss << "}";
        // 写入节点定义，使用 std::quoted 处理标签中的特殊字符
        file << "  node" << node->node_id << " [label=" << std::quoted(label_ss.str()) << "];" << std::endl;
    }

    // 从每个根节点开始，递归写入边信息
    for (const auto& root : roots_) {
        saveNodeToFile(file, root);
    }

    file << "}" << std::endl; // DOT 文件尾
    file.close(); // 关闭文件
}

// 实现 saveNodeToFile 辅助函数
void HypergraphTreeIndex::saveNodeToFile(std::ofstream& file, TreeNodePtr node) const {
    if (!node) return; // 空节点直接返回

    // 遍历当前节点的所有子节点
    for (const auto& child : node->children) {
        if (child) {
            // 写入连接父子节点的边
            file << "  node" << node->node_id << " -- node" << child->node_id << ";" << std::endl;
            // 递归处理子节点
            saveNodeToFile(file, child);
        }
    }
}

#endif // HYPERGRAPH_TREE_INDEX_H