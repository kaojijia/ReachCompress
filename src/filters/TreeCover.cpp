#include <vector>

#include "TreeCover.h"

using namespace std;

void TreeCover::offline_industry() 
{
    uint32_t num = g.vertices.size();
    this->tree_nodes = new TreeNode*[num];
    for(uint32_t index = 0; index < num; index++){
        tree_nodes[index] = new TreeNode;
        tree_nodes[index]->min_postorder=-1;
        tree_nodes[index]->postorder=-1;
        tree_nodes[index]->next=nullptr;
        tree_nodes[index]->tree_id=-1;
    }

    int tree_num = 0; 
    for(uint32_t index = 0; index < num; index++){
        uint32_t order = 1;
        if(g.vertices[index].in_degree==0&&g.vertices[index].out_degree==0){
            continue;
        }
        if(tree_nodes[index]->tree_id==-1){
            uint32_t x = post_traverse(index, tree_num, order);
            tree_num++;
        }
    }
}

uint32_t TreeCover::post_traverse(uint32_t index, uint32_t tree_num, uint32_t& order){
    uint32_t num = g.vertices.size();
    this->tree_nodes[index]->tree_id=tree_num;
    uint32_t min_order = 100000000;
    //看邻居节点的状态
    for(auto neighbour:g.vertices[index].LOUT){
        //没有被遍历过，就直接设值,
        if(this->tree_nodes[neighbour]->tree_id==-1){
            uint32_t temp = this->post_traverse(neighbour, tree_num, order);
            min_order = (temp<min_order)?temp:min_order;
        }
        //有被遍历过，但是是当前的子树就不管了（非树边）
        else if(this->tree_nodes[neighbour]->tree_id==tree_num){
            continue;
        }
        //有被遍历过，但是不是当前子树，多加一个(先不加了，减少空间占用)
        else{
            continue;
        }
    }
    this->tree_nodes[index]->postorder=order;
    this->tree_nodes[index]->min_postorder=(min_order<order)?min_order:order;
    order++;
    return this->tree_nodes[index]->min_postorder;
}


void TreeCover::printIndex(){
    for(int32_t i = 0; i < g.vertices.size(); i++){
        if (tree_nodes[i] != nullptr) {
            cout << "Index: " << i 
                 << ", Tree ID: " << tree_nodes[i]->tree_id 
                 << ", Min Postorder: " << tree_nodes[i]->min_postorder 
                 << ", Postorder: " << tree_nodes[i]->postorder 
                 << endl;
        } else {
            cout << "Index: " << i << " is nullptr" << endl;
        }
    }
}


bool TreeCover::reachability_query(int source, int target)
{
    for(auto source_ptr = tree_nodes[source]; source_ptr!=nullptr; source_ptr=source_ptr->next)
    {
        for(auto target_ptr = tree_nodes[target]; target_ptr!=nullptr; target_ptr=target_ptr->next){
            //不是同一个生成树的
            if(source_ptr->tree_id!=target_ptr->tree_id)
                continue;
            if(source_ptr->min_postorder<=target_ptr->min_postorder && target_ptr->postorder<=source_ptr->postorder)
                return true;
        }
    }
    return false;
}

