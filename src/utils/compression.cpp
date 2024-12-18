#include "compression.h"
#include <algorithm>
#include <stack>
#include <unordered_set>
#include <iostream>

Compression::Compression(const Graph& graph) : g(graph) {
    inNout1.clear();
    in1outN.clear();
    in1out1.clear();
    degree2.clear();
    for (size_t i = 0; i < g.vertices.size(); ++i) {
        if (g.vertices[i].LIN.empty() && g.vertices[i].LOUT.empty()) {
            continue;
        }
        classifyVertex(i);
    }
}


const Graph& Compression::getGraph() const {
    return g;
}

// 返回 mapping 的常量引用
const std::unordered_map<int, NodeMapping>& Compression::getMapping() const {
    return mapping;
}


// 查找节点的映射关系
int Compression::findMapping(int node) const {
    for (const auto& pair : mapping) {
        const NodeMapping& nodeMapping = pair.second;
        if (std::find(nodeMapping.sccMapping.begin(), nodeMapping.sccMapping.end(), node) != nodeMapping.sccMapping.end() ||
            std::find(nodeMapping.outMapping.begin(), nodeMapping.outMapping.end(), node) != nodeMapping.outMapping.end() ||
            std::find(nodeMapping.inMapping.begin(), nodeMapping.inMapping.end(), node) != nodeMapping.inMapping.end()) {
            return pair.first;
        }
    }
    return node; // If not found, return the node itself
}


// 压缩成有向无环子图
void Compression::compressRAG() {
    
}

// 删除节点
void Compression::removeNode(int node) {
    g.removeNode(node);
}

// 删除边
void Compression::removeEdge(int u, int v) {
    g.removeEdge(u,v);
}

// 核心压缩方法
void Compression::compress(int u, int v) {
    
    
}


// 归类节点，先删除再添加
void Compression::classifyVertex(int vertex) {
    inNout1.erase(vertex);
    in1outN.erase(vertex);
    in1out1.erase(vertex);
    degree2.erase(vertex);
    int in_degree = g.vertices[vertex].LIN.size();
    int out_degree = g.vertices[vertex].LOUT.size();

    if (out_degree == 1 && in_degree > 1) {
        inNout1.insert(vertex);
    } else if (in_degree == 1 && out_degree > 1) {
        in1outN.insert(vertex);
    } else if (in_degree == 1 && out_degree == 1) {
        in1out1.insert(vertex);
    } else if (out_degree == 2 || in_degree == 2) {
        degree2.insert(vertex);
    } 
}



/// @brief 按u->v->w的顺序合并节点 v的入度合并到w上，出度合并到u上
/// @param u 起点
/// @param v 中点
/// @param w 终点
void Compression::mergeNodes(int u, int v, int w){

    mergeNodes(u, v, false, false);
    mergeNodes(w, v, true, false);
    mapping.erase(v);
    removeNode(v);
    removeRedundantEdges(u, w);
}

/// @brief 合并两个节点、节点内的映射以及节点的边
/// @param u 起点
/// @param v 终点
/// @param is_reversed 是否反向（v指向u） 
/// @param is_delete 合并后是否删除v在图中和mapping中的记录
void Compression::mergeNodes(int u, int v ,bool is_reversed, bool is_delete) {
    if (g.vertices[u].LOUT.empty() && g.vertices[u].LIN.empty()) {
        throw std::runtime_error("Merge failed: Node u is isolated or not exist.");
    }
    if (g.vertices[v].LOUT.empty() && g.vertices[v].LIN.empty()) {
        throw std::runtime_error("Merge failed: Node v is isolated or not exist.");
    }
    
    if (!is_reversed) {
        // 如果u指向v
        mapping[u].outMapping.push_back(v);
        //添加v的出边到u上，u给自己加出边
        for (int neighbor : g.vertices[v].LOUT) {
            g.addEdge(u, neighbor);
        }

    } else if (is_reversed) {
        // v指向u的情况
        mapping[u].inMapping.push_back(v);
        // 添加v的入边到u上，u给自己加入边
        for (int neighbor : g.vertices[v].LIN) {
        g.addEdge(neighbor, u);
        }
    }

    //u要合并v的映射关系
    for (int neighbor : mapping[v].outMapping) {
        mapping[u].outMapping.push_back(neighbor);
    }
    for(int neighbor : mapping[v].inMapping){
        mapping[u].inMapping.push_back(neighbor);
    }
    for(int neighbor : mapping[v].sccMapping){
        mapping[u].sccMapping.push_back(neighbor);
    }
    if (is_delete)
    {
        mapping.erase(v);
        g.removeNode(v);
    }
    

}

//从匹配到的第二个边开始删除
void Compression::removeRedundantEdges(int u, int w) {
    // Remove redundant edges from u to w
    auto& u_out = g.vertices[u].LOUT;
    int count = 0;
    for (auto it = u_out.begin(); it != u_out.end(); ) {
        if (*it == w) {
            count++;
            if (count > 1) {
                it = u_out.erase(it);
                g.vertices[u].out_degree--;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    // Remove redundant edges from w to u
    auto& w_in = g.vertices[w].LIN;
    count = 0;
    for (auto it = w_in.begin(); it != w_in.end(); ) {
        if (*it == u) {
            count++;
            if (count > 1) {
                it = w_in.erase(it);
                g.vertices[w].in_degree--;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}


// 合并 in1out1 节点
// 看节点的前驱点和后继点
// 如果除去对方之外
// 分别只有这一个出度和入度的话可以合并
void Compression::mergeIn1Out1Nodes() {
    // std::vector<int> nodesToRemove;
    int flag = 0;
    int count = 0;
    while (!in1out1.empty()) {
        
        flag = 0;
        int node = *in1out1.begin();
        in1out1.erase(in1out1.begin());
        //if (g.vertices[node].LIN.size() == g.vertices[node].LOUT.size() == 1) {
            int predecessor = g.vertices[node].LIN.front();
            int successor = g.vertices[node].LOUT.front();

            if (g.hasEdge(predecessor, successor)) {
                flag = 1;
            }
            if (g.vertices[predecessor].LOUT.size()-flag == 1) {
                mergeNodes(predecessor, node, false, true);                
                in1out1.erase(in1out1.begin());
                removeRedundantEdges(predecessor, successor);
                classifyVertex(predecessor);
                count++;
                // std::cout<<"处理完成"<<++count<<"个节点"<<std::endl;
            } else if(g.vertices[successor].LIN.size()-flag == 1){
                mergeNodes(successor,node,true, true);
                in1out1.erase(in1out1.begin());
                removeRedundantEdges(predecessor, successor);
                classifyVertex(successor);
                count++;
                // std::cout<<"处理完成"<<++count<<"个节点"<<std::endl;
            }
        //}
        // auto i = g.statics();
        // std::cout <<"nodes num:"<< i.first << " " <<"edges num:" <<i.second << std::endl;s
            
    }
    std::cout<<"处理完成"<<count<<"个节点，mergeIn1Out1函数退出"<<std::endl;
}

void Compression::del_nodes(){

    std::vector<int> del_list;
    for(int i = 0; i < g.vertices.size();i++){
        if (g.vertices[i].in_degree ==0 && g.vertices[i].out_degree ==0)
        {
            continue;
        }
        if(g.vertices[i].in_degree ==0||g.vertices[i].out_degree==0){
            del_list.push_back(i);
        }
        
    }
    std::cout<<"无入度或出度的点数量是"<<del_list.size()<<std::endl;
    for(auto i:del_list){
        g.removeNode(i);
    }
}


// 中继点合并
// 某个点的一个in和一个out可以合并
// if 前驱和后继只有除了相互之间 之外的那么一个出度
// 扫描所有vertex的出度和入度，O(|V|+|E|)的时间
// 或者从out1集合中出发，找终点的终点只有一个入度的
// 再从out2集合中出发，找终点的终点和自己的另一个终点一样的
void Compression::mergeRouteNodes(){
    int flag=0;
    int count =0;
    //一个度的
    while (!inNout1.empty()) {
        // flag++;
        int node = *inNout1.begin();
        int route = g.vertices[node].LOUT.front();
        int successor = -1;

        inNout1.erase(inNout1.begin());

        //临时条件，中继点度数不能大于3
        if((g.vertices[route].in_degree + g.vertices[route].out_degree) > 3) continue;

        //找一个中继点后面只有一个入度的点拿来合并
        for(auto i: g.vertices[route].LOUT){
            if(g.vertices[i].LIN.size() == 1){
                successor = i;
                break;
            }
        }
        if(successor == -1){
            continue;
        }
        mergeNodes(node, route, successor);
        classifyVertex(node);
        classifyVertex(successor);
        std::cout<<"处理完成"<<++count<<"个节点"<<std::endl;
    }    
    std::cout<<"处理完成"<<count<<"个节点，mergeRouteNodes函数退出"<<std::endl;

    //2个度的

}
