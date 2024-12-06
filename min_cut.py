import os
import networkx as nx
from networkx.algorithms.flow import minimum_cut

def read_edges_from_file(file_path):
    """
    读取边文件，并返回边列表。
    
    :param file_path: 边文件路径
    :return: edges (列表)
    """
    edges = []
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) >= 2:
                try:
                    u, v = map(int, parts)
                    if u != v:  # 忽略自环
                        edges.append((u, v))
                except ValueError:
                    print(f"Skipping invalid line: {line.strip()}")
    return edges

def build_undirected_graph(edges):
    """
    构建无向图，并为每条边添加默认的容量属性。
    
    :param edges: 边列表
    :return: 无向图对象
    """
    G = nx.Graph()
    for u, v in edges:
        G.add_edge(u, v, capacity=1)  # 为每条边添加默认的容量属性
    return G

def min_cut_partition(G):
    """
    使用最小割算法将图分成两个部分。
    
    :param G: 图对象
    :return: 分区字典
    """
    # 选择两个种子节点
    nodes = list(G.nodes())
    seed1, seed2 = nodes[0], nodes[1]
    
    # 计算最小割
    cut_value, (reachable, non_reachable) = minimum_cut(G, seed1, seed2)
    
    # 构建分区字典
    partition = {}
    for node in reachable:
        partition[node] = 0
    for node in non_reachable:
        partition[node] = 1
    
    return partition

def write_partitions_to_file(partition, output_file):
    """
    将社区分区结果写入文件。
    
    :param partition: 分区字典
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for node, community_id in partition.items():
            file.write(f"{node} {community_id}\n")

def main(input_dir, output_dir):
    """
    处理输入目录下的所有边文件，进行社区检测并保存分区结果。
    
    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_partitions_mincut.txt")
        
        print(f"Processing file: {input_file}")

        # 读取边
        edges = read_edges_from_file(input_file)
        print(f"Total edges: {len(edges)}")

        # 构建无向图
        G = build_undirected_graph(edges)
        print("Graph constructed.")

        # 社区检测
        partition = min_cut_partition(G)
        num_communities = len(set(partition.values()))
        print(f"Detected {num_communities} communities.")

        # 写入分区结果
        write_partitions_to_file(partition, output_file)
        print(f"Partition results written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/medium"      # 输入边文件目录
    output_directory = "Partitions"       # 输出分区结果目录
    main(input_directory, output_directory)