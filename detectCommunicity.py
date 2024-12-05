import os
from infomap import Infomap

def read_edges_from_file(file_path):
    """
    读取边文件，并返回边列表。
    
    :param file_path: 边文件路径
    :return: edges (列表)
    """
    edges = []
    with open(file_path, 'r', encoding='utf-8-sig') as file:
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

def build_infomap_network(edges, batch_size=10000):
    """
    构建 Infomap 网络。
    
    :param edges: 边列表
    :param batch_size: 每批处理的边数
    :return: Infomap 网络对象
    """
    network = Infomap(directed=True,silent=True)
    for i in range(0, len(edges), batch_size):
        batch = edges[i:i + batch_size]
        for u, v in batch:
            network.addLink(u, v)
        network.run()
    return network

def detect_communities(network):
    """
    使用 Infomap 进行社区检测。
    
    :param network: Infomap 网络对象
    :return: Infomap 网络对象（包含社区信息）
    """
    try:
        network.run()
    except Exception as e:
        print(f"Error during community detection: {e}")
    return network

def write_partitions_to_file(network, output_file):
    """
    将社区分区结果写入文件。
    
    :param network: Infomap 网络对象
    :param output_file: 输出文件路径
    """
    with open(output_file, 'w') as file:
        for node in network.iterTree():
            if node.isLeaf():
                file.write(f"{node.physicalId} {node.moduleIndex()}\n")

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
        output_file = os.path.join(output_dir, f"{os.path.splitext(filename)[0]}_partitions.txt")
        
        print(f"Processing file: {input_file}")

        # 读取边
        edges = read_edges_from_file(input_file)
        print(f"Total edges: {len(edges)}")

        # 构建 Infomap 网络
        network = build_infomap_network(edges)
        print("Network constructed.")

        # 社区检测
        detect_communities(network)
        num_communities = len(set(node.moduleIndex() for node in network.iterTree() if node.isLeaf()))
        print(f"Detected {num_communities} communities.")

        # 写入分区结果
        write_partitions_to_file(network, output_file)
        print(f"Partition results written to: {output_file}\n")

    print("All files processed.")

if __name__ == "__main__":
    input_directory = "Edges/medium"      # 输入边文件目录
    output_directory = "Partitions"       # 输出分区结果目录
    main(input_directory, output_directory)