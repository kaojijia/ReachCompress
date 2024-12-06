import os
import networkx as nx
from community import community_louvain

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
    构建无向图。
    
    :param edges: 边列表
    :return: 无向图对象
    """
    G = nx.Graph()
    G.add_edges_from(edges)
    return G

def detect_communities(G, resolution_parameter=1.0, min_community_size=10):
    """
    使用 Louvain 方法进行社区检测，并合并小社区。
    
    :param G: 无向图对象
    :param resolution_parameter: 分辨率参数
    :param min_community_size: 最小社区大小
    :return: 分区字典
    """
    try:
        partition = community_louvain.best_partition(G, resolution=resolution_parameter)
        
        # 统计每个社区的大小
        community_sizes = {}
        for node, community in partition.items():
            if community not in community_sizes:
                community_sizes[community] = 0
            community_sizes[community] += 1
        
        # 合并小社区
        small_communities = {community for community, size in community_sizes.items() if size < min_community_size}
        if small_communities:
            # 找到最大的社区
            largest_community = max(community_sizes, key=community_sizes.get)
            for node, community in partition.items():
                if community in small_communities:
                    partition[node] = largest_community
        
    except Exception as e:
        print(f"Error during community detection: {e}")
        partition = {}
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

def multi_level_partition(G, resolution_parameter=1.0, min_community_size=10):
    """
    使用多层次方法进行社区检测。
    
    :param G: 无向图对象
    :param resolution_parameter: 分辨率参数
    :param min_community_size: 最小社区大小
    :return: 分区字典
    """
    partition = detect_communities(G, resolution_parameter, min_community_size)
    
    # 对每个分区进行细分
    sub_partitions = {}
    for community in set(partition.values()):
        subgraph_nodes = [node for node in G.nodes() if partition[node] == community]
        subgraph = G.subgraph(subgraph_nodes)
        sub_partition = detect_communities(subgraph, resolution_parameter, min_community_size)
        for node, sub_community in sub_partition.items():
            sub_partitions[node] = (community, sub_community)
    
    return sub_partitions

def write_multi_level_partitions_to_file(partition, output_file):
    """
    将多层次社区分区结果写入文件。
    
    :param partition: 分区字典
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for node, (community, sub_community) in partition.items():
            file.write(f"{node} {community} {sub_community}\n")

def main(input_dir, output_dir, resolution_parameter=1.0, min_community_size=10):
    """
    处理输入目录下的所有边文件，进行社区检测并保存分区结果。
    
    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    :param resolution_parameter: 分辨率参数
    :param min_community_size: 最小社区大小
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_partitions_louvain.txt")
        
        print(f"Processing file: {input_file}")

        # 读取边
        edges = read_edges_from_file(input_file)
        print(f"Total edges: {len(edges)}")

        # 构建无向图
        G = build_undirected_graph(edges)
        print("Graph constructed.")

        # 社区检测
        partition = multi_level_partition(G, resolution_parameter, min_community_size)
        num_communities = len(set(community for community, _ in partition.values()))
        print(f"Detected {num_communities} communities.")

        # 写入分区结果
        write_multi_level_partitions_to_file(partition, output_file)
        print(f"Partition results written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/medium"      # 输入边文件目录
    output_directory = "Partitions"       # 输出分区结果目录
    resolution_parameter = 0.7              # 分辨率参数
    min_community_size = 100               # 最小社区大小
    main(input_directory, output_directory, resolution_parameter, min_community_size)