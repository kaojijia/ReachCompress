import os
import networkx as nx

def ensure_folder_exists(folder):
    """确保文件夹存在，如果不存在则创建它。"""
    if not os.path.exists(folder):
        os.makedirs(folder)

def compute_strongly_connected_components(citation_graph, output_file_prefix):
    """计算并保存强连通分量"""
    strongly_connected_components = [c for c in nx.strongly_connected_components(citation_graph) if len(c) > 1]
    num_strongly_connected_components = len(strongly_connected_components)

    # 计算被包含在强连通分量中的节点数
    nodes_in_scc = set()
    for component in strongly_connected_components:
        nodes_in_scc.update(component)

    num_nodes_in_scc = len(nodes_in_scc)
    
    # 总节点数和边数
    total_nodes = citation_graph.number_of_nodes()
    total_edges = citation_graph.number_of_edges()
    
    # 强连通分量的节点比例和分量数比例
    ratio_nodes_in_scc = num_nodes_in_scc / total_nodes if total_nodes > 0 else 0
    ratio_scc_to_total = num_strongly_connected_components / total_nodes if total_nodes > 0 else 0

    # 将强连通分量结果保存到文件
    scc_output_file = f"{output_file_prefix}_strong_components.txt"
    with open(scc_output_file, 'w', encoding='utf-8') as out_file:
        out_file.write(f"点数: {total_nodes}\n")
        out_file.write(f"边数: {total_edges}\n")
        out_file.write(f"强连通分量数: {num_strongly_connected_components}\n")
        out_file.write(f"包含在强连通分量内的点数: {num_nodes_in_scc}\n")
        out_file.write(f"包含在强连通分量内的点数比率: {ratio_nodes_in_scc:.4f}\n")
        out_file.write(f"强连通分量数/总点数: {ratio_scc_to_total:.4f}\n")
    print(f"强连通分量结果已保存到 {scc_output_file}")

def compute_weakly_connected_components(citation_graph, output_file_prefix):
    """计算并保存弱连通分量"""
    weakly_connected_components = [c for c in nx.weakly_connected_components(citation_graph) if len(c) > 1]
    num_weakly_connected_components = len(weakly_connected_components)

    # 计算被包含在弱连通分量中的节点数
    nodes_in_wcc = set()
    for component in weakly_connected_components:
        nodes_in_wcc.update(component)

    num_nodes_in_wcc = len(nodes_in_wcc)
    
    # 总节点数和边数
    total_nodes = citation_graph.number_of_nodes()
    total_edges = citation_graph.number_of_edges()
    
    # 弱连通分量的节点比例和分量数比例
    ratio_nodes_in_wcc = num_nodes_in_wcc / total_nodes if total_nodes > 0 else 0
    ratio_wcc_to_total = num_weakly_connected_components / total_nodes if total_nodes > 0 else 0

    # 将弱连通分量结果保存到文件
    wcc_output_file = f"{output_file_prefix}_weak_components.txt"
    with open(wcc_output_file, 'w', encoding='utf-8') as out_file:
        out_file.write(f"点数: {total_nodes}\n")
        out_file.write(f"边数: {total_edges}\n")
        out_file.write(f"弱连通分量数: {num_weakly_connected_components}\n")
        out_file.write(f"包含在弱连通分量内的点数: {num_nodes_in_wcc}\n")
        out_file.write(f"包含在弱连通分量内的点数比率: {ratio_nodes_in_wcc:.4f}\n")
        out_file.write(f"弱连通分量数/总点数: {ratio_wcc_to_total:.4f}\n")
    print(f"弱连通分量结果已保存到 {wcc_output_file}")

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

if __name__ == "__main__":
    edges_folder = 'Edges/large'
    components_folder = 'Components'

    # 确保 Components 文件夹存在
    ensure_folder_exists(components_folder)

    # 处理 Edges 文件夹中的所有边集文件
    for edge_file in os.listdir(edges_folder):
        edge_file_path = os.path.join(edges_folder, edge_file)
        component_output_prefix = os.path.join(components_folder, os.path.splitext(edge_file)[0])
        
        # 检查是否已经存在强连通分量的结果文件
        scc_output_file = f"{component_output_prefix}_strong_components.txt"
        wcc_output_file = f"{component_output_prefix}_weak_components.txt"

        if os.path.exists(scc_output_file):
            print(f"强连通分量文件已存在，跳过: {scc_output_file}")
        else:
            # 加载边集并构建图
            citation_graph = load_graph_from_edges_file(edge_file_path)
            
            # 计算并保存强连通分量信息
            compute_strongly_connected_components(citation_graph, component_output_prefix)

        # # 检查是否已经存在弱连通分量的结果文件
        # if os.path.exists(wcc_output_file):
        #     print(f"弱连通分量文件已存在，跳过: {wcc_output_file}")
        # else:
        #     # 计算并保存弱连通分量信息
        #     compute_weakly_connected_components(citation_graph, component_output_prefix)
