import os
import networkx as nx
import leidenalg as la
from igraph import Graph

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

def build_directed_graph(edges):
    """
    构建有向图。
    
    :param edges: 边列表
    :return: 有向图对象
    """
    G = nx.DiGraph()
    G.add_edges_from(edges)
    return G

def detect_communities(G, resolution_parameter=1.0, max_communities=10):
    """
    使用 Leiden 方法进行社区检测，并尝试控制分区数量。
    
    :param G: 有向图对象
    :param resolution_parameter: 分辨率参数
    :param max_communities: 最大分区数量
    :return: 分区字典
    """
    try:
        # 将 NetworkX 图转换为 igraph 图
        ig_graph = Graph.TupleList(G.edges(), directed=True)
        # 使用 Leiden 算法进行社区检测
        partition = la.find_partition(ig_graph, la.CPMVertexPartition, resolution_parameter=resolution_parameter)
        while len(set(partition.membership)) > max_communities:
            resolution_parameter *= 1.1  # 增加分辨率参数以减少分区数量
            partition = la.find_partition(ig_graph, la.CPMVertexPartition, resolution_parameter=resolution_parameter)
        # 将分区结果转换为字典
        partition_dict = {node: partition.membership[i] for i, node in enumerate(G.nodes())}
    except Exception as e:
        print(f"Error during community detection: {e}")
        partition_dict = {}
    return partition_dict

def write_partitions_to_file(partition, output_file):
    """
    将社区分区结果写入文件。
    
    :param partition: 分区字典
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for node, community_id in partition.items():
            file.write(f"{node} {community_id}\n")

def main(input_dir, output_dir, resolution_parameter=1.0, max_communities=10):
    """
    处理输入目录下的所有边文件，进行社区检测并保存分区结果。
    
    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    :param resolution_parameter: 分辨率参数
    :param max_communities: 最大分区数量
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_partitions_leiden.txt")
        
        print(f"Processing file: {input_file}")

        # 读取边
        edges = read_edges_from_file(input_file)
        print(f"Total edges: {len(edges)}")

        # 构建有向图
        G = build_directed_graph(edges)
        print("Graph constructed.")

        # 社区检测
        partition = detect_communities(G, resolution_parameter, max_communities)
        num_communities = len(set(partition.values()))
        print(f"Detected {num_communities} communities.")

        # 写入分区结果
        write_partitions_to_file(partition, output_file)
        print(f"Partition results written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/medium"      # 输入边文件目录
    output_directory = "Partitions"       # 输出分区结果目录
    resolution_parameter = 1.0              # 初始分辨率参数
    max_communities = 50                  # 最大分区数量
    main(input_directory, output_directory, resolution_parameter, max_communities)