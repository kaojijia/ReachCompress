import os
import networkx as nx

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

def find_reachable_pairs_within_distance(G, distances):
    """
    查找在给定距离内的可达点对。
    
    :param G: 有向图对象
    :param distances: 查询距离列表
    :return: 字典，键为距离，值为点对列表
    """
    pairs_within_distance = {distance: [] for distance in distances}
    
    for node in G.nodes():
        lengths = nx.single_source_shortest_path_length(G, node, cutoff=max(distances))
        for target, length in lengths.items():
            if length in distances:
                pairs_within_distance[length].append((node, target))
    
    return pairs_within_distance

def write_pairs_to_file(pairs_within_distance, output_file):
    """
    将点对写入文件。
    
    :param pairs_within_distance: 字典，键为距离，值为点对列表
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for distance, pairs in pairs_within_distance.items():
            file.write(f"Distance {distance}:\n")
            for u, v in pairs:
                file.write(f"{u} {v}\n")
            file.write("\n")

def main(input_dir, output_dir, distances):
    """
    处理输入目录下的所有边文件，查找在给定距离内的可达点对并保存结果。
    
    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    :param distances: 查询距离列表
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_distance_pairs.txt")
        
        print(f"Processing file: {input_file}")

        # 读取边
        edges = read_edges_from_file(input_file)
        print(f"Total edges: {len(edges)}")

        # 构建有向图
        G = build_directed_graph(edges)
        print("Graph constructed.")

        # 查找在给定距离内的可达点对
        pairs_within_distance = find_reachable_pairs_within_distance(G, distances)
        print("Pairs found.")

        # 写入点对结果
        write_pairs_to_file(pairs_within_distance, output_file)
        print(f"Pairs written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/generate"      # 输入边文件目录
    output_directory = "Pairs"              # 输出点对结果目录
    distances = [2, 4, 6, 8, 10]            # 查询距离列表
    main(input_directory, output_directory, distances)