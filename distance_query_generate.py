import os
import random
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

def build_graph(edges):
    """
    构建无向图。

    :param edges: 边列表
    :return: 无向图对象
    """
    G = nx.Graph()
    G.add_edges_from(edges)
    return G

def find_random_pairs(G, distances, max_pairs_per_distance, reachable_ratio, trials=1000):
    """
    随机查找点对，每个距离记录1个点对，重复指定次数。

    :param G: 图对象
    :param distances: 查询距离列表
    :param max_pairs_per_distance: 每个距离的最大点对数量
    :param reachable_ratio: 可达点对的比例
    :param trials: 随机尝试次数
    :return: 字典，键为距离，值为点对列表
    """
    pairs_within_distance = {distance: [] for distance in distances}
    reachable_pairs = {distance: [] for distance in distances}
    nodes = list(G.nodes())

    for _ in range(trials):
        # 随机选择起点
        u = random.choice(nodes)
        lengths = nx.single_source_shortest_path_length(G, u, cutoff=max(distances))
        
        for distance in distances:
            if distance in lengths:
                v = lengths.get(distance)
                if v is not None:
                    pair = (u, v)
                    if pair not in reachable_pairs[distance]:
                        reachable_pairs[distance].append(pair)
                        pairs_within_distance[distance].append(pair)
                        if len(pairs_within_distance[distance]) >= max_pairs_per_distance * reachable_ratio:
                            break
        # 检查是否已经达到所有距离的可达点对要求
        if all(len(pairs_within_distance[d]) >= max_pairs_per_distance * reachable_ratio for d in distances):
            break

    # 生成不可达点对
    for distance in distances:
        required_unreachable = max_pairs_per_distance - len(pairs_within_distance[distance])
        if required_unreachable > 0:
            attempts = 0
            max_attempts = required_unreachable * 10
            while required_unreachable > 0 and attempts < max_attempts:
                u, v = random.sample(nodes, 2)
                if not G.has_edge(u, v) and not nx.has_path(G, u, v) and (u, v) not in pairs_within_distance[distance]:
                    pairs_within_distance[distance].append((u, v))
                    required_unreachable -= 1
                attempts += 1

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

def process_file(input_file, output_file, distances, max_pairs_per_distance, reachable_ratio):
    """
    处理单个文件，查找在给定距离内的点对并保存结果。

    :param input_file: 输入文件路径
    :param output_file: 输出文件路径
    :param distances: 查询距离列表
    :param max_pairs_per_distance: 每个距离的最大点对数量
    :param reachable_ratio: 可达点对的比例
    """
    print(f"Processing file: {input_file}")

    # 读取边
    edges = read_edges_from_file(input_file)
    print(f"Total edges: {len(edges)}")

    # 构建图
    G = build_graph(edges)
    print("Graph constructed.")

    # 查找在给定距离内的点对
    pairs_within_distance = find_random_pairs(G, distances, max_pairs_per_distance, reachable_ratio)
    print("Pairs found.")

    # 写入点对结果
    write_pairs_to_file(pairs_within_distance, output_file)
    print(f"Pairs written to: {output_file}\n")

def main(input_dir, output_dir, distances, max_pairs_per_distance, reachable_ratio, trials=1000):
    """
    处理输入目录下的所有边文件，查找在给定距离内的点对并保存结果。

    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    :param distances: 查询距离列表
    :param max_pairs_per_distance: 每个距离的最大点对数量
    :param reachable_ratio: 可达点对的比例
    :param trials: 随机尝试次数
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    files = os.listdir(input_dir)
    for filename in files:
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_distance_pairs.txt")
        process_file(input_file, output_file, distances, max_pairs_per_distance, reachable_ratio)

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/DAGs/large"         # 输入边文件目录
    output_directory = "QueryPairs"               # 输出点对结果目录
    distances = [6, 8, 10]                         # 查询距离列表
    max_pairs_per_distance = 100                  # 每个距离的最大点对数量
    reachable_ratio = 0.5                          # 可达点对的比例
    trials = 100                                   # 随机尝试次数
    main(
        input_directory,
        output_directory,
        distances,
        max_pairs_per_distance,
        reachable_ratio,
        trials
    )