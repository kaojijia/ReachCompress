import os
import networkx as nx
from networkx.algorithms.flow import minimum_cut
import random

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
                    u, v = map(int, parts[:2])  # 只取前两个元素作为节点
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

def distribute_partitions(total, parts):
    """
    将总分区数尽量均匀分配到各个部分，同时确保不会超过最大限制。

    :param total: 总分区数
    :param parts: 需要分配的部分数
    :return: 每部分分配的分区数的列表
    """
    if parts == 0:
        return []
    base = total // parts
    remainder = total % parts
    distribution = [base] * parts
    for i in range(remainder):
        distribution[i] += 1
    return distribution

def select_seed_nodes(G):
    """
    选择两个种子节点，用于计算最小割。
    可以根据需要调整种子节点选择策略。

    :param G: 图对象
    :return: 两个种子节点
    """
    # 示例策略：随机选择两个不同的节点
    nodes = list(G.nodes())
    if len(nodes) < 2:
        return nodes[0], nodes[0]
    seed1, seed2 = random.sample(nodes, 2)
    return seed1, seed2

def min_cut_partition(G, desired_partitions=2, current_partitions=None, partition_id=0, min_size=10):
    """
    使用最小割算法将图分割成多个部分，同时确保每个分区至少有 min_size 个顶点。

    :param G: 图对象
    :param desired_partitions: 期望的分区数
    :param current_partitions: 当前的分区字典
    :param partition_id: 当前分区的ID
    :param min_size: 最小分区大小
    :return: 分区字典
    """
    if current_partitions is None:
        current_partitions = {}

    total_nodes = len(G.nodes())
    max_possible_partitions = total_nodes // min_size
    if desired_partitions > max_possible_partitions:
        print(f"调整期望分区数从 {desired_partitions} 到 {max_possible_partitions} 以满足最小分区大小要求。")
        desired_partitions = max_possible_partitions

    # 基本情况：如果只需要一个分区，直接分配所有节点到当前分区
    if desired_partitions == 1:
        for node in G.nodes():
            current_partitions[node] = partition_id
        return current_partitions

    # 确保当前子图足够大，可以继续分割
    if len(G.nodes()) < min_size * desired_partitions:
        # 将所有节点分配到当前分区
        for node in G.nodes():
            current_partitions[node] = partition_id
        return current_partitions

    # 选择两个种子节点
    seed1, seed2 = select_seed_nodes(G)

    # 计算最小割
    try:
        cut_value, (reachable, non_reachable) = minimum_cut(G, seed1, seed2)
    except Exception as e:
        print(f"Minimum cut failed for partition_id {partition_id}: {e}")
        # 将所有节点分配到当前分区
        for node in G.nodes():
            current_partitions[node] = partition_id
        return current_partitions

    # 检查子图大小是否满足最小分区大小
    reachable_size = len(reachable)
    non_reachable_size = len(non_reachable)

    if reachable_size < min_size or non_reachable_size < min_size:
        # 无法满足最小分区大小，尝试其他种子节点
        # 如果仍然无法分割，分配到当前分区
        print(f"分割生成的子图大小不满足最小分区要求 (reachable: {reachable_size}, non_reachable: {non_reachable_size})。")
        for node in G.nodes():
            current_partitions[node] = partition_id
        return current_partitions

    # 分配分区ID
    new_partition_id = partition_id
    old_partition_id = partition_id + 1
    for node in reachable:
        current_partitions[node] = new_partition_id
    for node in non_reachable:
        current_partitions[node] = old_partition_id

    # 递增分区ID
    partition_id += 2

    # 计算剩余需要分区的数量
    remaining_partitions = desired_partitions - 2

    if remaining_partitions <= 0:
        return current_partitions

    # 将每个分区进一步分割
    subgraphs = [G.subgraph(reachable).copy(), G.subgraph(non_reachable).copy()]
    sub_desired_list = distribute_partitions(remaining_partitions, len(subgraphs))

    for subgraph, sub_desired in zip(subgraphs, sub_desired_list):
        if len(subgraph.nodes()) < min_size:
            # 无法进一步分割，分配到当前分区
            print(f"子图大小小于最小分区大小 ({len(subgraph.nodes())} < {min_size})，分配到分区ID {partition_id}。")
            for node in subgraph.nodes():
                current_partitions[node] = partition_id
            partition_id += 1
            continue
        # 调用递归分割
        current_partitions = min_cut_partition(subgraph, sub_desired, current_partitions, partition_id, min_size)
        partition_id += sub_desired

    return current_partitions

def write_partitions_to_file(partition, output_file):
    """
    将社区分区结果写入文件。

    :param partition: 分区字典
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for node, community_id in sorted(partition.items()):
            file.write(f"{node} {community_id}\n")

def main(input_dir, output_dir, desired_partitions=4, min_size=10):
    """
    处理输入目录下的所有边文件，进行社区检测并保存分区结果。

    :param input_dir: 输入目录路径
    :param output_dir: 输出目录路径
    :param desired_partitions: 期望的分区数
    :param min_size: 每个分区的最小顶点数量
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
        partition = min_cut_partition(G, desired_partitions=desired_partitions, min_size=min_size)
        num_communities = len(set(partition.values()))
        print(f"Detected {num_communities} communities.")

        # 统计每个分区的大小
        partition_sizes = {}
        for node, comm_id in partition.items():
            partition_sizes.setdefault(comm_id, 0)
            partition_sizes[comm_id] += 1

        print("Partition sizes:")
        for comm_id, size in partition_sizes.items():
            print(f"  Partition {comm_id}: {size} nodes")
        print()

        # 检查每个分区是否满足最小大小要求
        all_meet_min_size = all(size >= min_size for size in partition_sizes.values())
        if not all_meet_min_size:
            print(f"Warning: Not all partitions meet the minimum size of {min_size} nodes.")

        # 写入分区结果
        write_partitions_to_file(partition, output_file)
        print(f"Partition results written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/large"      # 输入边文件目录
    output_directory = "Partitions"       # 输出分区结果目录
    desired_partitions = 6                # 期望分区数
    min_size = 5                         # 每个分区的最小顶点数量
    main(input_directory, output_directory, desired_partitions, min_size)