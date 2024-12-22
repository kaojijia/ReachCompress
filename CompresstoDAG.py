import networkx as nx
import os

def read_edges_from_file(file_path):
    edges = []
    with open(file_path, 'r') as file:
        for line in file:
            u, v = map(int, line.strip().split())
            edges.append((u, v))
    return edges

def write_edges_to_file(edges, file_path):
    with open(file_path, 'w') as file:
        for u, v in edges:
            file.write(f"{u} {v}\n")

def write_mapping_to_file(mapping, file_path):
    with open(file_path, 'w') as file:
        for node, scc_id in mapping.items():
            file.write(f"{node} {scc_id}\n")

def compress_scc(graph):
    scc = list(nx.strongly_connected_components(graph))
    scc_map = {}
    compressed_graph = nx.DiGraph()

    for component in scc:
        representative = next(iter(component))
        for node in component:
            scc_map[node] = representative

    for u, v in graph.edges():
        compressed_u = scc_map[u]
        compressed_v = scc_map[v]
        if compressed_u != compressed_v:
            compressed_graph.add_edge(compressed_u, compressed_v)

    return compressed_graph, scc_map

def process_file(input_file, output_dir):
    edges = read_edges_from_file(input_file)
    graph = nx.DiGraph(edges)

    compressed_graph, scc_map = compress_scc(graph)

    base_name = os.path.splitext(os.path.basename(input_file))[0]
    output_edges_file = os.path.join(output_dir, f"{base_name}_DAG")
    output_mapping_file = os.path.join(output_dir, f"{base_name}_mapping")

    write_edges_to_file(compressed_graph.edges(), output_edges_file)
    write_mapping_to_file(scc_map, output_mapping_file)

    print(f"Compressed graph edges written to: {output_edges_file}")
    print(f"Node mapping written to: {output_mapping_file}")

        # 计算压缩后图的节点数和边数
    num_nodes = compressed_graph.number_of_nodes()
    num_edges = compressed_graph.number_of_edges()

    # 打印节点数和边数
    print(f"Number of nodes in the compressed graph: {num_nodes}")
    print(f"Number of edges in the compressed graph: {num_edges}")
    print("----------------------------------------")

def main(input_dir, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        input_file = os.path.join(input_dir, filename)
        if os.path.isfile(input_file):
            process_file(input_file, output_dir)

if __name__ == "__main__":
    input_dir = "Edges/test"  # 输入边文件目录
    output_dir = "Edges/DAGs/test"  # 输出目录
    main(input_dir, output_dir)