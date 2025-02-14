#include "SetSearch.h"
#include "pll.h"
#include "TreeCover.h"

SetSearch::SetSearch(Graph &g)
{
    this->g = std::make_shared<Graph>(g);
    this->csr = std::make_shared<CSRGraph>();
    this->csr->fromGraph(*this->g);
    this->pll = std::make_shared<PLL>(*this->g);
    this->tree_cover = std::make_shared<TreeCover>(*this->g);
    in_key_points.resize(this->g->get_num_vertices() + 10);
    out_key_points.resize(this->g->get_num_vertices() + 10);
    topo_level.resize(this->g->get_num_vertices() + 10, -1);
}
SetSearch::~SetSearch()
{
    this->g.reset();
    this->csr.reset();
    this->pll.reset();
}

void SetSearch::offline_industry()
{
    this->pll->offline_industry();

    // 为每个顶点构建关键路标点索引
    build_key_points(num_key_points);

    // 为每个顶点构建拓扑层级的索引
    build_topo_level();

    // 为每个顶点构建n个生成树的索引
    //  build_tree_index();
    this->tree_cover->offline_industry();
}

void SetSearch::build_key_points(int num)
{
    // 使用优先队列来存储节点及其度数
    std::priority_queue<std::pair<int, int>> pq; // first: degree, second: node id

    // 遍历所有节点，计算度数并加入优先队列
    for (size_t i = 0; i < this->g->get_num_vertices(); ++i)
    {
        int degree = this->g->vertices[i].in_degree + this->g->vertices[i].out_degree;
        if (degree == 0)
            continue;
        pq.push({degree, static_cast<int>(i)});
    }

    // 取出前 num 个最大度数的节点
    std::vector<int> key_points;
    while (!pq.empty() && key_points.size() < static_cast<size_t>(num))
    {
        key_points.push_back(pq.top().second);
        pq.pop();
    }

    // 确保返回的 vector 长度为 num
    if (key_points.size() < num)
    {
        key_points.resize(num, -1); // 如果节点数不足，用 -1 填充
    }
    // 确保pop出来的是按照度数从大到小的顺序
    reverse(key_points.begin(), key_points.end());

    // 从每个关键点出发做正向和反向bfs
    // 如果正向bfs遍历到一个节点i，就把关键点加入到节点的in集合中，也就是in_key_points[i]中
    // 对于反向bfs也是一样， 遍历到一个节点i，就把关键点加入到节点的out集合中，也就是out_key_points[i]中
    while (!key_points.empty())
    {
        int key_point = key_points.back();
        key_points.pop_back();

        // 正向bfs
        queue<int> q;
        q.push(key_point);
        vector<int> visited(this->g->vertices.size(), 0);
        while (!q.empty())
        {
            int cur = q.front();
            q.pop();
            visited[cur] = 1;
            for (auto i : this->g->vertices[cur].LOUT)
            {
                if (visited[i])
                    continue;
                in_key_points[i].insert(key_point);
                q.push(i);
            }
        }
        // 反向bfs
        q.push(key_point);
        visited = vector<int>(this->g->vertices.size(), 0);
        while (!q.empty())
        {
            int cur = q.front();
            q.pop();
            visited[cur] = 1;
            for (auto i : this->g->vertices[cur].LIN)
            {
                if (visited[i])
                    continue;
                out_key_points[i].insert(key_point);
                q.push(i);
            }
        }
    }
}

void SetSearch::build_topo_level()
{

    shared_ptr<Graph> temp_graph = make_shared<Graph>(false);
    for (int i = 0; i < this->g->vertices.size(); i++)
    {
        if (this->g->vertices[i].in_degree != 0 || this->g->vertices[i].out_degree != 0)
        {
            for (auto in_neighbour : this->g->vertices[i].LIN)
            {
                temp_graph->addEdge(in_neighbour, i);
            }
            for (auto out_neighbour : this->g->vertices[i].LOUT)
            {
                temp_graph->addEdge(i, out_neighbour);
            }
        }
    }

    // 收集和分配
    int level = 0;
    vector<int> removed_nodes(0);
    int num_nodes = temp_graph->get_num_vertices();
    while (num_nodes > removed_nodes.size())
    {
        vector<int> remove_nodes(0);

        for (int i = 0; i < this->g->vertices.size(); i++)
        {
            if (temp_graph->vertices[i].in_degree == 0 && temp_graph->vertices[i].out_degree != 0 && find(removed_nodes.begin(), removed_nodes.end(), i) == removed_nodes.end())
            {
                remove_nodes.push_back(i);
            }
            // 节点没有度（被减没了），但是有入边
            if (temp_graph->vertices[i].in_degree == 0 && temp_graph->vertices[i].out_degree == 0 && !temp_graph->vertices[i].LIN.empty() && find(removed_nodes.begin(), removed_nodes.end(), i) == removed_nodes.end())
            {
                remove_nodes.push_back(i);
            }
        }
        for (auto i : remove_nodes)
        {
            this->topo_level[i] = level;
            // 不去做移除，就是把后继点的入度往下减
            for (auto out_neighbour : temp_graph->vertices[i].LOUT)
            {
                temp_graph->vertices[out_neighbour].in_degree--;
            }
            removed_nodes.push_back(i);
        }
        level++;
    }
    temp_graph.reset();
}

vector<pair<int, int>> SetSearch::set_reachability_query(vector<int> source_set, vector<int> target_set)
{
    // 对source_set和target_set进行合并，减少集合大小
    // 1、两个set过来先找等价点
    set<int> source_set_eq;
    set<int> target_set_eq;
    map<int, set<int>> source_map;
    map<int, set<int>> target_map;
    // O(m)
    for (auto i : source_set)
    {
        if (this->g->vertices[i].equivalance != 999999999)
        {
            source_set_eq.insert(this->g->vertices[i].equivalance);
            source_map[this->g->vertices[i].equivalance].insert(i);
        }
        else
        {
            source_set_eq.insert(i);
        }
    }
    // O(n)
    for (auto i : target_set)
    {
        if (this->g->vertices[i].equivalance != 999999999)
        {
            target_set_eq.insert(this->g->vertices[i].equivalance);
            target_map[this->g->vertices[i].equivalance].insert(i);
        }
        else
        {
            target_set_eq.insert(i);
        }
    }

    // 2、TODO：为两个set分别构建生成树区间索引，为每两个节点找最近的公共父节点

    // 3、为两个set分别构建支配树，上层节点查找可达后，底层节点再去找可达

    // 构建森林
    auto source_forest = build_source_forest(source_set_eq);
    auto target_forest = build_target_forest(target_set_eq);

    // 初始化优先队列
    // O(n*m)
    std::priority_queue<QueueItem> queue;
    for (auto &s_root : source_forest)
    {
        for (auto &t_root : target_forest)
        {
            // if (topo_level[s_root->id] > topo_level[t_root->id])
            //     continue;
            queue.push({s_root, t_root, s_root->depth + t_root->depth});
        }
    }

    vector<pair<int, int>> results;
    unordered_map<size_t, bool> cache;

    cout << queue.size() << endl;

    while (!queue.empty())
    {
        auto item = queue.top();
        queue.pop();

        auto s_node = item.s_node;
        auto t_node = item.t_node;

        // 生成缓存键
        size_t key = (reinterpret_cast<size_t>(s_node.get()) << 32) | reinterpret_cast<size_t>(t_node.get());

        bool reachable = false;
        if (cache.count(key))
        {
            reachable = cache[key];
        }
        else
        {
            if (topo_level[s_node->id] > topo_level[t_node->id])
                reachable = false;
            else
                reachable = reachability_query(s_node->id, t_node->id);
            cache[key] = reachable;
        }

        if (reachable)
        {
            // 记录所有覆盖对
            for (int s : s_node->covered_nodes)
                for (int t : t_node->covered_nodes)
                    results.emplace_back(s, t);
        }
        else
        {
            // 展开子节点组合
            // 两个节点都有子节点，将所有子节点的笛卡尔积入队
            if (!s_node->children.empty() && !t_node->children.empty())
            {
                for (auto &s_child : s_node->children)
                {
                    for (auto &t_child : t_node->children)
                    {
                        if (topo_level[s_child->id] > topo_level[t_child->id])
                            continue;
                        queue.push({s_child, t_child,
                                    s_child->depth + t_child->depth});
                    }
                }
            }
            else
            {
                // 处理单边展开
                if (!s_node->children.empty())
                {
                    for (auto &s_child : s_node->children)
                    {
                        if (topo_level[s_child->id] > topo_level[t_node->id])
                            continue;
                        queue.push({s_child, t_node,
                                    s_child->depth + t_node->depth});
                    }
                }
                if (!t_node->children.empty())
                {
                    for (auto &t_child : t_node->children)
                    {
                        // if (topo_level[s_node->id] > topo_level[t_child->id])
                        //     continue;
                        queue.push({s_node, t_child,
                                    s_node->depth + t_child->depth});
                    }
                }
            }
        }
    }

    // 去重处理
    sort(results.begin(), results.end());
    results.erase(unique(results.begin(), results.end()), results.end());
    return results;
}

bool SetSearch::reachability_query(int source, int target)
{

    // 拓扑层级过滤
    if (topo_level[source] > topo_level[target])
        return false;

    return this->pll->query(source, target);
}

std::vector<std::pair<std::string, std::string>> SetSearch::getIndexSizes() const
{
    return std::vector<std::pair<std::string, std::string>>();
}
vector<SetSearch::ForestNodePtr> SetSearch::build_source_forest(const set<int> &nodes)
{
    vector<ForestNodePtr> forest;
    TopoLevelComparator cmp(topo_level);

    // 初始化节点为一系列叶子节点并入队
    list<ForestNodePtr> node_list;
    for (int v : nodes)
    {
        auto node = make_shared<ForestNode>(v);
        node->covered_nodes.insert(v);
        node->key_points = std::vector<int>(out_key_points[v].begin(), out_key_points[v].end()); // 加载预计算的关键点
        node_list.push_back(node);
    }

    // 合并循环
    while (!node_list.empty())
    {
        auto current = node_list.front();
        node_list.pop_front();
        bool merged = false;

        auto it = node_list.begin();
        while (it != node_list.end())
        {
            auto node = *it;
            // 双指针法查找共同关键点
            int cp = find_common_keypoint(current->key_points, node->key_points);

            if (cp != -1)
            {
                // 创建父节点（使用实际找到的关键点）
                auto parent = make_shared<ForestNode>(cp);
                parent->children.push_back(current);
                parent->children.push_back(node);
                parent->covered_nodes.insert(current->covered_nodes.begin(),
                                             current->covered_nodes.end());
                parent->covered_nodes.insert(node->covered_nodes.begin(),
                                             node->covered_nodes.end());

                // 合并关键点（取并集后重新排序）
                vector<int> merged_kps;
                set_union(current->key_points.begin(), current->key_points.end(),
                          node->key_points.begin(), node->key_points.end(),
                          back_inserter(merged_kps), cmp);

                parent->key_points = merged_kps;

                node_list.push_front(parent);
                it = node_list.erase(it);
                merged = true;
                break;
            }
            else
            {
                ++it;
            }
        }

        if (!merged)
        {
            forest.push_back(current);
        }
    }

    for (auto &root : forest)
    {
        establish_parent_links(root);
    }

    return forest;
}

vector<SetSearch::ForestNodePtr> SetSearch::build_target_forest(const set<int> &nodes)
{
    vector<ForestNodePtr> forest;
    TopoLevelComparator cmp(topo_level);

    // 初始化目标节点列表
    list<ForestNodePtr> node_list;
    for (int v : nodes)
    {
        auto node = make_shared<ForestNode>(v);
        node->covered_nodes.insert(v);
        node->key_points = std::vector<int>(in_key_points[v].begin(), in_key_points[v].end()); // 使用入方向关键点
        node_list.push_back(node);
    }

    // 合并循环（与源森林对称但方向相反）
    while (!node_list.empty())
    {
        auto current = node_list.front();
        node_list.pop_front();
        bool merged = false;

        auto it = node_list.begin();
        while (it != node_list.end())
        {
            auto node = *it;

            // 查找共同前驱关键点（使用in_key_points）
            int cp = find_common_keypoint(current->key_points, node->key_points);

            if (cp != -1)
            {
                // 创建父节点（使用实际前驱ID）
                auto parent = make_shared<ForestNode>(cp);

                // 维护层级关系（父在前，子在后）
                if (topo_level[parent->id] <= topo_level[current->id])
                {
                    parent->children.push_back(current);
                    parent->children.push_back(node);
                }
                else
                {
                    // 处理异常情况（理论上不应出现）
                    throw logic_error("Invalid topology hierarchy");
                }

                // 合并覆盖节点
                parent->covered_nodes.insert(current->covered_nodes.begin(),
                                             current->covered_nodes.end());
                parent->covered_nodes.insert(node->covered_nodes.begin(),
                                             node->covered_nodes.end());

                // 合并关键点（保持有序性）
                vector<int> merged_kps;
                set_union(current->key_points.begin(), current->key_points.end(),
                          node->key_points.begin(), node->key_points.end(),
                          back_inserter(merged_kps), cmp);
                parent->key_points = merged_kps;

                // 更新列表
                node_list.push_front(parent);
                it = node_list.erase(it);
                merged = true;
                break;
            }
            else
            {
                ++it;
            }
        }

        if (!merged)
        {
            forest.push_back(current);
        }
    }

    // 后处理：建立父指针
    for (auto &root : forest)
    {
        establish_parent_links(root);
    }

    return forest;
}

// 辅助函数：建立父节点链接
void SetSearch::establish_parent_links(ForestNodePtr node)
{
    for (auto &child : node->children)
    {
        child->parent = node;
        establish_parent_links(child);
    }
}

// 公共关键点查找函数
int SetSearch::find_common_keypoint(const vector<int> &kps1,
                                    const vector<int> &kps2)
{
    vector<int> result;
    int i = 0, j = 0;
    while (i < kps1.size() && j < kps2.size())
    {
        if (kps1[i] == kps2[j])
        {
            result.push_back(kps1[i]);
            ++i;
            ++j;
        }
        if (topo_level[kps1[i]] > topo_level[kps2[j]])
        {
            ++i;
        }
        else
        {
            ++j;
        }
    }

    if (result.empty())
        return -1;
    // 这里看看有没有优化
    return result[0];
}