#include "SetSearch.h"
#include "pll.h"
#include "TreeCover.h"
#define DEBUG
struct ReachRecord
{
    int source_id;
    int target_id;
    int source_degree;
    int target_degree;
    unsigned long source_OUT_size;
    unsigned long target_IN_size;
    signed long query_time;

    bool operator<(const ReachRecord &other) const
    {
        return query_time < other.query_time;
    }
    bool operator>(const ReachRecord &other) const
    {
        return query_time > other.query_time;
    }

    bool operator==(const ReachRecord &other) const
    {
        return query_time == other.query_time;
    }
};

SetSearch::SetSearch(Graph &g, int num_key_points)
{
    this->g = std::make_shared<Graph>(g);
    this->csr = std::make_shared<CSRGraph>();
    this->csr->fromGraph(*this->g);
    this->pll = std::make_shared<PLL>(*this->g);
    this->tree_cover = std::make_shared<TreeCover>(*this->g);
    in_key_points.resize(this->g->vertices.size() + 10);
    out_key_points.resize(this->g->vertices.size() + 10);
    topo_level.resize(this->g->vertices.size() + 10, 999999999);
    this->num_key_points = num_key_points;
}
SetSearch::~SetSearch()
{
    this->g.reset();
    this->csr.reset();
    this->pll.reset();
}

void SetSearch::offline_industry()
{
#ifdef DEBUG
    auto start = std::chrono::high_resolution_clock::now();
#endif
    this->pll->offline_industry();
#ifdef DEBUG
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << Algorithm::getCurrentTimestamp() << "pll索引构建用时 " << duration << " 微秒" << std::endl;
#endif
    // 为每个顶点构建关键路标点索引
    build_key_points(num_key_points);

    // 为每个顶点构建拓扑层级的索引
    // build_topo_level();
#ifdef DEBUG
    start = std::chrono::high_resolution_clock::now();
#endif
    build_topo_level_optimized();
#ifdef DEBUG
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << Algorithm::getCurrentTimestamp() << "topo层级索引构建用时 " << duration << " 微秒" << std::endl;
#endif

    // 为每个顶点构建n个生成树的索引
    //  build_tree_index();
    // this->tree_cover->offline_industry();
}

void SetSearch::build_key_points(int num)
{
    // 使用优先队列来存储节点及其度数
    std::priority_queue<std::pair<int, int>> pq; // first: degree, second: node id

    // 遍历所有节点，计算度数并加入优先队列
    for (size_t i = 0; i < this->g->vertices.size(); ++i)
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

    // 确保pop出来的是按照度数从大到小的顺序
    reverse(key_points.begin(), key_points.end());
    this->key_points = key_points;
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

void SetSearch::build_topo_level_optimized()
{
    shared_ptr<Graph> temp_graph = make_shared<Graph>(false);
    int n = this->g->vertices.size();
    for (int i = 0; i < n; i++)
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

    // 初始化 topo_level 数组（默认值 999999999 或其它大数）
    this->topo_level.assign(n, 999999999);

    // 使用队列进行层次遍历：先将所有满足 in_degree==0 的节点入队
    std::queue<int> q;
    for (int i = 0; i < n; i++)
    {
        // 按原代码判断条件：如果节点出度不为 0 或者虽然度为0但有入边，则放入队列
        if (temp_graph->vertices[i].in_degree == 0 &&
            (temp_graph->vertices[i].out_degree != 0 || !temp_graph->vertices[i].LIN.empty()))
        {
            q.push(i);
        }
    }

    int level = 0;
    while (!q.empty())
    {
        int size = q.size();
        // 当前层的所有节点统一赋予同一层级
        for (int i = 0; i < size; i++)
        {
            int node = q.front();
            q.pop();
            this->topo_level[node] = level;
            // 对当前节点的每个后继，将其 in_degree 减 1
            for (auto out_neighbour : temp_graph->vertices[node].LOUT)
            {
                temp_graph->vertices[out_neighbour].in_degree--;
                // 当后继节点 in_degree 减为 0 时，加入下一层队列
                if (temp_graph->vertices[out_neighbour].in_degree == 0)
                {
                    q.push(out_neighbour);
                }
            }
        }
        level++;
    }
    temp_graph.reset();
}

vector<pair<int, int>> SetSearch::set_reachability_query(vector<int> source_set, vector<int> target_set)
{

#ifdef DEBUG
    auto start = std::chrono::high_resolution_clock::now();
#endif
    u_int32_t count = 0;
    // 对source_set和target_set进行合并，减少集合大小
    // 1、两个set过来先找等价点，并过滤不存在的点
    set<int> source_set_eq;
    set<int> target_set_eq;

    map<int, set<int>> source_map;
    map<int, set<int>> target_map;

    // O(m)
    for (auto i : source_set)
    {
        if (i > this->g->vertices.size())
            continue;
        if (this->g->vertices[i].out_degree == 0)
        {
            continue;
        }
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
        if (i > this->g->vertices.size())
            continue;
        if (this->g->vertices[i].in_degree == 0)
        {
            continue;
        }
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
    unordered_set<int> source_set_unordered(source_set.begin(), source_set.end());
    unordered_set<int> target_set_unordered(target_set.begin(), target_set.end());

    // 2、TODO：为两个set分别构建生成树区间索引，为每两个节点找最近的公共父节点

    // 3、为两个set分别构建支配树，上层节点查找可达后，底层节点再去找可达

    // 构建森林
    auto source_forest = build_source_forest(source_set_eq);
    auto target_forest = build_target_forest(target_set_eq);

    // 存储要匹配的值
    // O(n*m)
    // 双端队列
    deque<pair<ForestNodePtr, ForestNodePtr>> queue;
    for (auto &s_root : source_forest)
    {
        for (auto &t_root : target_forest)
        {
            queue.emplace_back(s_root, t_root);
        }
    }

    vector<pair<int, int>> results;
    unordered_map<size_t, bool> cache;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << Algorithm::getCurrentTimestamp() << "在线构建索引用时 " << duration << " 微秒" << std::endl;

#ifdef DEBUG
    cout << Algorithm::getCurrentTimestamp() << "可达树的根节点笛卡尔积总数为：" << queue.size() << endl;
    std::chrono::_V2::system_clock::time_point start1, end1, start2, end2, start3, end3, start4, end4, start5, end5;
    int64_t duration1 = 0, duration2 = 0, duration3 = 0, duration4 = 0, duration5 = 0;
    int topo_count = 0;
    vector<ReachRecord> reachable_records;
#endif
    int loop_count = 0;
    while (!queue.empty())
    {
        loop_count++;
        auto item = queue.front();
        queue.pop_front();

        auto s_node = item.first;
        auto t_node = item.second;

        // 生成缓存键
        size_t key = (reinterpret_cast<size_t>(s_node.get()) << 32) | reinterpret_cast<size_t>(t_node.get());

        bool reachable = false;
        if (cache.count(key))
        {
            continue;
        }
        else
        {
            cache[key] = true;

            // 拓扑层级过滤
            if (topo_level[s_node->id] > topo_level[t_node->id])
            {
                reachable = false;
                topo_count++;
            }
            else
            {
#ifdef DEBUG
                start1 = std::chrono::high_resolution_clock::now();
#endif
                count++;
                reachable = reachability_query(s_node->id, t_node->id);
                // reachable_records.emplace_back(ReachRecord{s_node->id, t_node->id, this->g->vertices[s_node->id].out_degree, this->g->vertices[t_node->id].in_degree, this->pll->OUT[s_node->id].size(), this->pll->IN[t_node->id].size(), count});
#ifdef DEBUG
                end1 = std::chrono::high_resolution_clock::now();
                auto temp_duration = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();
                duration1 += temp_duration;
                reachable_records.emplace_back(ReachRecord{s_node->id, t_node->id, this->g->vertices[s_node->id].out_degree, this->g->vertices[t_node->id].in_degree, this->pll->OUT[s_node->id].size(), this->pll->IN[t_node->id].size(), temp_duration});
#endif
            }
        }

        if (reachable)
        {
#ifdef DEBUG
            start2 = std::chrono::high_resolution_clock::now();
#endif
            // 记录所有覆盖对
            for (int s : s_node->covered_nodes)
                for (int t : t_node->covered_nodes)
                {
                    if (source_set_unordered.find(s) == source_set_unordered.end() || target_set_unordered.find(t) == target_set_unordered.end())
                    {
                        continue; // 如果遍历到的不属于要查询的点，就不要进入结果集了
                    }
                    if (s == t)
                        continue;
                    results.emplace_back(s, t);
                }
#ifdef DEBUG
            end2 = std::chrono::high_resolution_clock::now();
            duration2 += std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();
#endif
        }
        else
        {
#ifdef DEBUG
            start3 = std::chrono::high_resolution_clock::now();
#endif
            // 展开子节点组合
            // 两个节点都有子节点，将所有子节点的笛卡尔积入队
            if (!s_node->children.empty() && !t_node->children.empty())
            {
                // 先将根节点和对面的子节点入队

                for (auto &s_child : s_node->children)
                {
                    for (auto &t_child : t_node->children)
                    {
                        queue.emplace_front(s_child, t_child);
                    }
                }
                for (auto &s_child : s_node->children)
                {
                    queue.emplace_front(s_child, t_node);
                }
                for (auto &t_child : t_node->children)
                {
                    queue.emplace_front(s_node, t_child);
                }
            }
            else
            {
                // 处理单边展开
                if (!s_node->children.empty())
                {
                    for (auto &s_child : s_node->children)
                    {
                        queue.emplace_front(s_child, t_node);
                    }
                }
                if (!t_node->children.empty())
                {
                    for (auto &t_child : t_node->children)
                    {
                        queue.emplace_front(s_node, t_child);
                    }
                }
            }
#ifdef DEBUG
            end3 = std::chrono::high_resolution_clock::now();
            duration3 += std::chrono::duration_cast<std::chrono::microseconds>(end3 - start3).count();
#endif
        }
    }

#ifdef DEBUG
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    cout << Algorithm::getCurrentTimestamp() << "进入查询循环 " << loop_count << "次" << endl;
    cout << Algorithm::getCurrentTimestamp() << "拓扑排序过滤 " << topo_count << "次" << endl;
    cout << Algorithm::getCurrentTimestamp() << "原始可达性查询次数 ：" << source_set.size() * target_set.size() << " 次 " << endl;
    cout << Algorithm::getCurrentTimestamp() << "压缩可达性查询次数  :" << count << " 次 " << endl;
    cout << Algorithm::getCurrentTimestamp() << "查询次数压缩为原来的 ：" << fixed << setprecision(3) << (double)count / (source_set.size() * target_set.size()) * 100 << " % " << endl;
    cout << Algorithm::getCurrentTimestamp() << "可达性查询耗时 " << duration1 << " 微秒" << endl;
    cout << Algorithm::getCurrentTimestamp() << "记录覆盖对耗时 " << duration2 << " 微秒" << endl;
    cout << Algorithm::getCurrentTimestamp() << "构建子节点组合耗时 " << duration3 << " 微秒" << endl;
    cout << Algorithm::getCurrentTimestamp() << "set_reachability_query用时 " << duration << " 微秒" << std::endl;
    // Save reachability records to a file
    std::ofstream record_file("reachability_records.csv");
    if (record_file.is_open())
    {
        // Write the title line
        record_file << "Source ID,Target ID,Source Degree,Target Degree,Source OUT Size,Target IN Size,Query Time (microseconds)" << std::endl;

        // Write the data lines
        for (const auto &record : reachable_records)
        {
            record_file << record.source_id << ","
                        << record.target_id << ","
                        << record.source_degree << ","
                        << record.target_degree << ","
                        << record.source_OUT_size << ","
                        << record.target_IN_size << ","
                        << record.query_time << std::endl;
        }
        record_file.close();
    }
    else
    {
        std::cerr << "Unable to open file for writing reachability records." << std::endl;
    }
#endif
    return results;
}

bool SetSearch::reachability_query(int source, int target)
{
    return this->pll->query(source, target);
}

std::vector<std::pair<std::string, std::string>> SetSearch::getIndexSizes() const
{
    return std::vector<std::pair<std::string, std::string>>();
}

vector<SetSearch::ForestNodePtr> SetSearch::build_forest(
    const set<int> &nodes, bool is_source)
{
    vector<ForestNodePtr> forest;
    unordered_map<int, ForestNodePtr> node_registry;
    const auto &key_points = is_source ? out_key_points : in_key_points;

    // 初始化节点并入队
    list<ForestNodePtr> node_list;
    for (int v : nodes)
    {
        auto node = make_shared<ForestNode>(v);
        node->covered_nodes.insert(v);
        node_registry[v] = node;
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
            auto peer = *it;

            // 获取关键点集
            const auto &kps_current = key_points[current->id];
            const auto &kps_peer = key_points[peer->id];

            int cp = find_common_keypoint(kps_current, kps_peer);

            if (cp != -1)
            {
                // 查找或创建父节点
                ForestNodePtr parent;
                auto parent_iter = node_registry.find(cp);

                if (parent_iter != node_registry.end())
                {
                    parent = parent_iter->second;
                }
                else
                {
                    parent = make_shared<ForestNode>(cp);
                    parent->covered_nodes.insert(cp);
                    node_registry[cp] = parent;
                }

                // 合并覆盖集
                parent->covered_nodes.insert(current->covered_nodes.begin(),
                                             current->covered_nodes.end());
                parent->covered_nodes.insert(peer->covered_nodes.begin(),
                                             peer->covered_nodes.end());

                // 建立父子关系
                parent->children.push_back(current);
                parent->children.push_back(peer);
                current->parent = parent;
                peer->parent = parent;

                // 更新工作队列
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

    // 后处理：收集孤立根节点
    unordered_set<int> valid_roots;
    for (auto &root : forest)
    {
        valid_roots.insert(root->id);
    }

    for (auto &[id, node] : node_registry)
    {
        if (!node->parent.lock() && !valid_roots.count(id))
        {
            forest.push_back(node);
        }
    }

    return forest;
}
vector<SetSearch::ForestNodePtr> SetSearch::build_forest_optimized(const set<int> &nodes, bool is_source)
{
    // 1. 为每个关键点建立一个根节点
    unordered_map<int, ForestNodePtr> forest_roots;
    for (int kp : this->key_points)
    {
        // 对于源点集合，这个关键点如果入度很少就不要参与运算了
        // 对于目标点集合，这个关键点如果出度很少就不要参与运算了
        int degree = is_source ? this->g->vertices[kp].in_degree : this->g->vertices[kp].out_degree;
        if(degree<50)continue;
        auto root = make_shared<ForestNode>(kp);
        root->covered_nodes.insert(kp); // 关键点自身加入覆盖集合
        forest_roots[kp] = root;
    }

    // 2. 关键点之间挂接：直接在比较中建立父子关系，不删除关键点，只作标记
    vector<int> keypoints_copy = this->key_points; // 复制关键点列表
    sort(keypoints_copy.begin(), keypoints_copy.end());
    // 这里不使用 erase 删除元素，而是仅建立挂接关系
    for (size_t i = 0; i < keypoints_copy.size(); ++i)
    {
        if(!forest_roots.count(keypoints_copy[i]))continue;
        for (size_t j = i + 1; j < keypoints_copy.size(); ++j)
        {
            if(!forest_roots.count(keypoints_copy[j]))continue;
            int kp1 = keypoints_copy[i];
            int kp2 = keypoints_copy[j];
            // 根据 is_source 选择对应的关键点集合：当 is_source 为 true 时使用 out_key_points，否则使用 in_key_points
            const set<int>& kps1 = is_source ? out_key_points[kp1] : in_key_points[kp1];

            if (kps1.count(kp2))
            {
                // kp1 包含 kp2，则将 kp1 挂接在 kp2 下
                forest_roots[kp2]->children.emplace_back(forest_roots[kp1]);
                forest_roots[kp1]->parent = forest_roots[kp2];
            }
            else
            {
                // 反向判断：如果 kp2 包含 kp1，则将 kp2 挂接在 kp1 下
                const set<int>& kps2 = is_source ? out_key_points[kp2] : in_key_points[kp2];
                if (kps2.count(kp1))
                {
                    forest_roots[kp1]->children.emplace_back(forest_roots[kp2]);
                    forest_roots[kp2]->parent = forest_roots[kp1];
                    break; // 因为是有向无环图，不会相互包含，无需继续比较
                }
            }
        }
    }

    // 3. 先构建两个结果容器：forest_keys 存关键点生成的树（根节点），isolated_forest 存孤立节点（非关键点）
    vector<ForestNodePtr> forest_keys;
    for (auto &p : forest_roots)
    {
        // 只有未挂接到其它关键点下的，作为关键点树的根节点
        if (!p.second->parent.lock())
            forest_keys.emplace_back(p.second);
    }
    vector<ForestNodePtr> isolated_forest;
    // 3. 对传入的普通节点，根据其关键点信息挂接为关键点树的子节点，否则作为孤立节点

    for (int v : nodes)
    {
        int flag = 0;
        const set<int>& node_kps = is_source ? out_key_points[v] : in_key_points[v];
        if (!node_kps.empty())
        {
            // 若节点有关键点信息，则挂接到对应的关键点树上
            for (int kp : node_kps)
            {
                if (forest_roots.count(kp))
                {
                    auto child = make_shared<ForestNode>(v);
                    child->covered_nodes.insert(v);
                    forest_roots[kp]->children.emplace_back(child);
                    child->parent = forest_roots[kp];
                    flag = 1;
                }
            }
        }
        else
        {
            // 否则，作为孤立节点（可以认为该节点无法挂接到任何关键点树上）
            if (this->g->isExist(v))
            {
                auto isolated = make_shared<ForestNode>(v);
                isolated->covered_nodes.insert(v);
                isolated_forest.emplace_back(isolated);
                flag = 1;
            }
        }
        // 遍历完所有关键点也无处可去的，就直接挂上去就好了
        if(flag == 0 && this->g->isExist(v))
        {
            auto isolated = make_shared<ForestNode>(v);
            isolated->covered_nodes.insert(v);
            isolated_forest.emplace_back(isolated);
            flag = 1;
        }
    }


    // 4. 最终结果：将关键点产生的森林放前面，孤立节点接在后面
    forest_keys.insert(forest_keys.end(), isolated_forest.begin(), isolated_forest.end());
    return forest_keys;
}

// 包装函数
vector<SetSearch::ForestNodePtr> SetSearch::build_source_forest(const set<int> &nodes)
{
    // return build_forest(nodes, true);
    return build_forest_optimized(nodes, true);
}

vector<SetSearch::ForestNodePtr> SetSearch::build_target_forest(const set<int> &nodes)
{
    // return build_forest(nodes, false);
    return build_forest_optimized(nodes, false);
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
int SetSearch::find_common_keypoint(const set<int> &kps1,
                                    const set<int> &kps2)
{
    vector<int> result;
    auto it1 = kps1.begin();
    auto it2 = kps2.begin();
    while (it1 != kps1.end() && it2 != kps2.end())
    {
        if (*it1 == *it2)
        {
            result.push_back(*it1);
            ++it1;
            ++it2;
        }
        else if (topo_level[*it1] > topo_level[*it2])
        {
            ++it1;
        }
        else
        {
            ++it2;
        }
    }

    if (result.empty())
        return -1;
    // 这里看看有没有优化
    return result[0];
}