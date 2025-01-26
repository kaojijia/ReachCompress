#include"SetSearch.h"

SetSearch::SetSearch(Graph &g) {
    this->g = std::make_shared<Graph>(g);
    this->csr = std::make_shared<CSRGraph>();
    this->csr->fromGraph(*this->g);
    this->pll = std::make_unique<PLL>(*this->g);
    in_key_points.resize(this->g->get_num_vertices());
    out_key_points.resize(this->g->get_num_vertices());
    topo_level.resize(this->g->get_num_vertices(),-1);
}
SetSearch::~SetSearch() {
    this->g.reset();
    this->csr.reset();
    this->pll.reset();
}

void SetSearch::offline_industry() {
    this->pll->offline_industry();

    //为每个顶点构建关键路标点索引
    build_key_points(num_key_points);

    //为每个顶点构建拓扑层级的索引
    build_topo_level();

    //为每个顶点构建n个生成树的索引
    build_tree_index();
}

void SetSearch::build_key_points(int num)
{
    // 使用优先队列来存储节点及其度数
    std::priority_queue<std::pair<int, int>> pq; // first: degree, second: node id

    // 遍历所有节点，计算度数并加入优先队列
    for (size_t i = 0; i < this->g->get_num_vertices(); ++i) {
        int degree = this->g->vertices[i].in_degree + this->g->vertices[i].out_degree;
        if(degree == 0)
            continue;
        pq.push({degree, static_cast<int>(i)});
    }

    // 取出前 num 个最大度数的节点
    std::vector<int> key_points;
    while (!pq.empty() && key_points.size() < static_cast<size_t>(num)) {
        key_points.push_back(pq.top().second);
        pq.pop();
    }

    // 确保返回的 vector 长度为 num
    if (key_points.size() < num) {
        key_points.resize(num, -1); // 如果节点数不足，用 -1 填充
    }
    // 确保pop出来的是按照度数从大到小的顺序
    reverse(key_points.begin(), key_points.end());

    // 从每个关键点出发做正向和反向bfs
    // 如果正向bfs遍历到一个节点i，就把关键点加入到节点的in集合中，也就是in_key_points[i]中
    // 对于反向bfs也是一样， 遍历到一个节点i，就把关键点加入到节点的out集合中，也就是out_key_points[i]中
    while(!key_points.empty()){
        int key_point = key_points.back();
        key_points.pop_back();

        //正向bfs
        queue<int> q;
        q.push(key_point);
        vector<int>visited(this->g->vertices.size(),0);
        while(!q.empty()){
            int cur = q.front();
            q.pop();
            visited[cur] = 1;
            for(auto i:this->g->vertices[cur].LOUT){
                if(visited[i])
                    continue;
                in_key_points[i].insert(key_point);
                q.push(i);

            }
        }
        //反向bfs
        q.push(key_point);
        visited = vector<int>(this->g->vertices.size(),0);
        while(!q.empty()){
            int cur = q.front();
            q.pop();
            visited[cur] = 1;
            for(auto i:this->g->vertices[cur].LIN){
                if(visited[i])
                    continue;
                out_key_points[i].insert(key_point);
                q.push(i);
            }
        }

    }

}

void SetSearch::build_topo_level()
{

    unique_ptr<Graph> temp_graph = make_unique<Graph>(false);
    for(int i = 0; i < this->g->vertices.size(); i++){
        if(this->g->vertices[i].in_degree != 0 || this->g->vertices[i].out_degree != 0){
            for(auto in_neighbour:this->g->vertices[i].LIN){
                temp_graph->addEdge(in_neighbour,i);
            }
            for(auto out_neighbour:this->g->vertices[i].LOUT){
                temp_graph->addEdge(i,out_neighbour);
            }
        }
    }
    //收集和分配
    int level = 0;
    while(temp_graph->get_num_vertices() > 0){
        vector<int> remove_nodes;
        for(int i = 0; i < this->g->vertices.size(); i++){
            if(temp_graph->vertices[i].in_degree == 0 && temp_graph->vertices[i].out_degree != 0){
                remove_nodes.push_back(i);
            }
            for(auto i:remove_nodes){
                this->topo_level[i] = level;
                temp_graph->removeNode(i);
            }
        }
        level++;
    }
    temp_graph.reset();
}

vector<pair<int, int>> SetSearch::set_reachability_query(vector<int> source_set, vector<int> target_set)
{
    //两个set过来先找等价点
    
    
    //为两个set分别构建支配树，上层节点查找可达后，底层节点再去找可达
    for(int i = 0; i < source_set.size(); i++)
    {


    }

    for(int i = 0; i < target_set.size(); i++)
    {
        
    }




    return vector<pair<int, int>>();
}



bool SetSearch::reachability_query(int source, int target) {
    
    //如果拓扑层序反向，那么直接不可达
    
    
    
    
    
    return this->pll->query(source, target);
}