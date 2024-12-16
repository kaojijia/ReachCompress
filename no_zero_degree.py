import networkx as nx

def load_graph_from_edges_file(edges_file):
    """从边集文件加载图"""
    citation_graph = nx.DiGraph()  # 使用有向图
    
    with open(edges_file, 'r', encoding='utf-8') as file:  # 使用 'utf-8' 处理 BOM
        print(f"加载边集文件: {edges_file}")
        for line in file:
            line = line.strip()
            if line:  # 确保行不为空
                source, target = line.split()  # 根据空格拆分
                citation_graph.add_edge(int(source.strip()), int(target.strip()))  # 添加边
    
    return citation_graph

def count_and_write_degree_nodes(graph, out_degree_file, in_degree_file):
    """统计入度不为0的顶点和出度不为0的顶点，并写入文件"""
    out_degree_nodes = [node for node, degree in graph.out_degree() if degree > 0]
    in_degree_nodes = [node for node, degree in graph.in_degree() if degree > 0]

    # 输出数量到控制台
    print(f"出度不为0的顶点数量: {len(out_degree_nodes)}")
    print(f"入度不为0的顶点数量: {len(in_degree_nodes)}")

    # 写入文件
    with open(out_degree_file, 'w') as out_file:
        out_file.write('\n'.join(map(str, out_degree_nodes)))

    with open(in_degree_file, 'w') as in_file:
        in_file.write('\n'.join(map(str, in_degree_nodes)))

if __name__ == "__main__":
    edges_file = './Edges/DAGs/large/tweibo-edgelist_DAG' 
    out_degree_file = './result/out_degree_nodes.txt'
    in_degree_file = './result/in_degree_nodes.txt'
    
    graph = load_graph_from_edges_file(edges_file)
    
    count_and_write_degree_nodes(graph, out_degree_file, in_degree_file)