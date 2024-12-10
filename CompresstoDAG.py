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

def main(input_file, output_dir):
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

if __name__ == "__main__":
    input_file = "Edges/large/tweibo-edgelist.txt"  # 输入边文件路径
    output_dir = "Edges/DAGs"     # 输出目录
    main(input_file, output_dir)