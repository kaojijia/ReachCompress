import os
import networkx as nx
from concurrent.futures import ThreadPoolExecutor, as_completed

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

def compute_reach_ratio(graph):
    """计算整个图中所有点对之间的可达性比例 (使用集合存储可达节点)"""
    n = graph.number_of_nodes()
    if n == 0:
        print("错误：图中没有顶点。")
        return 0.0

    # 获取拓扑排序
    try:
        topo_order = list(nx.topological_sort(graph))
    except nx.NetworkXUnfeasible:
        print("错误：输入图不是DAG。")
        return 0.0

    # 动态规划数组：记录每个节点的可达集合
    reachable = {node: set() for node in graph.nodes()}

    # 从拓扑排序的末尾向前处理
    for node in reversed(topo_order):
        for neighbor in graph.successors(node):
            reachable[node].update(reachable[neighbor])  # 合并后继节点的可达集合
        reachable[node].add(node)  # 加入当前节点本身

    # 计算总的可达点对数
    reachable_pairs = sum(len(reachable[node]) - 1 for node in graph.nodes())  # 减去自身

    # 计算可达性比例
    total_pairs = n * (n - 1)  # 排除自身可达的情况
    ratio = reachable_pairs / total_pairs if total_pairs > 0 else 0.0
    return ratio

def has_cycle(graph):
    """高效检测图中是否存在环"""
    return not nx.is_directed_acyclic_graph(graph)

def process_directory(path, output_file):
    """遍历路径下的所有文件并计算可达性比例，并将结果保存到文件中"""
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, _, files in os.walk(path):
            for file in files:
                edges_file = os.path.join(root, file)
                print(f"处理文件: {edges_file}")
                graph = load_graph_from_edges_file(edges_file)

                # 检测环并决定是否继续计算
                if has_cycle(graph):
                    print(f"输入图包含环，跳过文件: {edges_file}")
                    outfile.write(f"文件: {edges_file}, 错误: 输入图包含环\n")
                else:
                    reach_ratio = compute_reach_ratio(graph)
                    print(f"文件: {edges_file}, 可达性比例: {reach_ratio}")
                    outfile.write(f"文件: {edges_file}, 可达性比例: {reach_ratio}\n")

if __name__ == "__main__":
    directory_path = './Edges/generate' 
    output_file = './ratio_results_py.txt' 
    process_directory(directory_path, output_file)
