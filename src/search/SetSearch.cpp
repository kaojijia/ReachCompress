#include "SetSearch.h"
#include "pll.h"
#include "TreeCover.h"
#define DEBUG

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
    this->pll->offline_industry();

    // 为每个顶点构建关键路标点索引
    build_key_points(num_key_points);

    // 为每个顶点构建拓扑层级的索引
    build_topo_level();
    // build_topo_level_optimized();


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
    // 初始化 topological_levels 的大小并填充默认值
    topological_levels.assign(this->g->vertices.size(), -1);
    while (num_nodes > removed_nodes.size())
    {
        vector<int> remove_nodes(0);

        for (int i = 0; i < this->g->vertices.size(); i++)
        {
            if (temp_graph->vertices[i].in_degree == 0 && temp_graph->vertices[i].out_degree != 0 
                && find(removed_nodes.begin(), removed_nodes.end(), i) == removed_nodes.end())
            {
                remove_nodes.push_back(i);
            }
            // 节点没有度（被减没了），但是有入边
            if (temp_graph->vertices[i].in_degree == 0 && temp_graph->vertices[i].out_degree == 0 
                && !temp_graph->vertices[i].LIN.empty() 
                && find(removed_nodes.begin(), removed_nodes.end(), i) == removed_nodes.end())
            {
                remove_nodes.push_back(i);
            }
        }
        for (auto i : remove_nodes)
        {
            // 修改前：this->topo_level[i] = level;
            // 修改后：
            this->topological_levels[i] = level;
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


    auto start = std::chrono::high_resolution_clock::now();
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
        if(i > this->g->vertices.size()) continue;
        if(this->g->vertices[i].out_degree == 0){
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
        if(i > this->g->vertices.size()) continue;
        if( this->g->vertices[i].in_degree == 0){
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
    //双端队列
    deque<pair<ForestNodePtr, ForestNodePtr>> queue; 
    for (auto &s_root : source_forest) {
        for (auto &t_root : target_forest) {
            queue.emplace_back(s_root, t_root);
        }
    }

    vector<pair<int, int>> results;
    unordered_map<size_t, bool> cache;

    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "在线构建索引用时 " << duration << " 微秒" << std::endl;



#ifdef DEBUG
    cout <<"可达树的根节点笛卡尔积总数为："<< queue.size() << endl;
#endif

    while (!queue.empty())
    {
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
                reachable = false;
            else{
                count++;
                reachable = reachability_query(s_node->id, t_node->id);
            }
        }

        if (reachable)
        {
            // 记录所有覆盖对
            for (int s : s_node->covered_nodes)
                for (int t : t_node->covered_nodes){
                    if(source_set_unordered.find(s) == source_set_unordered.end() || target_set_unordered.find(t) == target_set_unordered.end()){
                        continue; // 如果遍历到的不属于要查询的点，就不要进入结果集了
                    }
                    if(s==t) continue;
                    results.emplace_back(s, t);
                }

        }
        else
        {
            // 展开子节点组合
            // 两个节点都有子节点，将所有子节点的笛卡尔积入队
            if (!s_node->children.empty() && !t_node->children.empty())
            {
                // 先将根节点和对面的子节点入队
                for (auto &s_child : s_node->children) {
                    queue.emplace_back(s_child, t_node);
                }
                for (auto &t_child : t_node->children) {
                    queue.emplace_back(s_node, t_child);
                }
                for (auto &s_child : s_node->children)
                {
                    for (auto &t_child : t_node->children)
                    {
                        queue.emplace_back(s_child, t_child);
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
                        queue.emplace_back(s_child, t_node);
                    }
                }
                if (!t_node->children.empty())
                {
                    for (auto &t_child : t_node->children)
                    {
                        queue.emplace_back(s_node, t_child);
                    }
                }
            }
        }
    }

    // // 去重处理
    sort(results.begin(), results.end());
    results.erase(unique(results.begin(), results.end()), results.end());
#ifdef DEBUG
    cout <<" 共比较次数  :" << count <<" 次 "<< endl;
    cout <<" 原始比较次数 ：" << source_set.size() * target_set.size() << " 次 " << endl;
    cout <<" 比较次数压缩为原来的 ：" << fixed << setprecision(3) << (double)count / (source_set.size() * target_set.size()) * 100 << " % " << endl;
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
    const set<int>& nodes, bool is_source) 
{
    vector<ForestNodePtr> forest;
    unordered_map<int, ForestNodePtr> node_registry;
    const auto& key_points = is_source ? out_key_points : in_key_points;

    // 初始化节点并入队
    list<ForestNodePtr> node_list;
    for (int v : nodes) {
        auto node = make_shared<ForestNode>(v);
        node->covered_nodes.insert(v);
        node_registry[v] = node;
        node_list.push_back(node);
    }

    // 合并循环
    while (!node_list.empty()) {
        auto current = node_list.front();
        node_list.pop_front();
        bool merged = false;

        auto it = node_list.begin();
        while (it != node_list.end()) {
            auto peer = *it;
            
            // 获取关键点集
            const auto& kps_current = key_points[current->id];
            const auto& kps_peer = key_points[peer->id];
            
            int cp = find_common_keypoint(kps_current, kps_peer);

            if (cp != -1) {
                // 查找或创建父节点
                ForestNodePtr parent;
                auto parent_iter = node_registry.find(cp);
                
                if (parent_iter != node_registry.end()) {
                    parent = parent_iter->second;
                } else {
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
            } else {
                ++it;
            }
        }

        if (!merged) {
            forest.push_back(current);
        }
    }

    // 后处理：收集孤立根节点
    unordered_set<int> valid_roots;
    for (auto& root : forest) {
        valid_roots.insert(root->id);
    }

    for (auto& [id, node] : node_registry) {
        if (!node->parent.lock() && !valid_roots.count(id)) {
            forest.push_back(node);
        }
    }

    return forest;
}

// 包装函数
vector<SetSearch::ForestNodePtr> SetSearch::build_source_forest(const set<int>& nodes) {
    return build_forest(nodes, true);
}

vector<SetSearch::ForestNodePtr> SetSearch::build_target_forest(const set<int>& nodes) {
    return build_forest(nodes, false);
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