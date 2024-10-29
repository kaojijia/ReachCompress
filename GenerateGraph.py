import networkx as nx
import matplotlib.pyplot as plt
import random
import os
import datetime
import matplotlib

# 设置 matplotlib 使用非交互后端
matplotlib.use('Agg')


# 参数设置
num_nodes = 40  # 图中的节点数
num_edges = 90  # 需要生成的边数
scc_ratio = 0  # 强连通分量的比例
num_scc = 4  # 强连通分量的数量


def generate_random_directed_graph(num_nodes, num_edges, scc_ratio, num_scc):
    G = nx.DiGraph()

    # 创建随机有向图节点
    for i in range(num_nodes):
        G.add_node(i)

    # 分配强连通分量的节点数
    num_scc_nodes = int(num_nodes * scc_ratio)

    # 记录已经添加的边数
    current_edges = 0

    # 创建强连通分量并添加边
    sccs = []
    for _ in range(num_scc):
        scc_size = num_scc_nodes // num_scc
        scc_nodes = random.sample(range(num_nodes), scc_size)
        sccs.append(scc_nodes)
        for i in range(scc_size):
            for j in range(scc_size):
                if i != j:
                    G.add_edge(scc_nodes[i], scc_nodes[j])
                    current_edges += 1

    # 添加剩余的随机边直到满足 num_edges 要求
    while current_edges < num_edges:
        u, v = random.sample(range(num_nodes), 2)
        if not G.has_edge(u, v):  # 避免重复添加边
            G.add_edge(u, v)
            current_edges += 1

    return G

def visualize_graph(G, filepath):
    """
    可视化有向图并保存为文件
    """
    pos = nx.spring_layout(G)  # 使用 spring_layout 布局
    plt.figure(figsize=(8, 6))
    nx.draw(G, pos, with_labels=True, node_size=500, node_color='skyblue', font_size=10, font_weight='bold', arrows=True)
    plt.savefig(filepath)  # 保存图片而不是显示
    print(f"图像已保存为 {filepath}")

def save_graph_as_edge_list(G, filepath):
    """
    将有向图的边列表存储为指定格式的txt文件
    """
    with open(filepath, 'w') as f:
        for u, v in G.edges():
            f.write(f"{u} {v}\n")
    print(f"边集已保存为 {filepath}")


# 获取当前日期时间作为文件名后缀
current_time = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

# 创建保存目录（如果目录不存在则创建）
graph_dir = "Graph"
edges_dir = "Edges"
os.makedirs(graph_dir, exist_ok=True)
os.makedirs(edges_dir, exist_ok=True)

# 生成文件名
graph_image_filename = os.path.join(graph_dir, f"gene_graph_{current_time}.png")
edges_filename = os.path.join(edges_dir,'generate', f"gene_edges_{current_time}")

# 生成随机有向图
G = generate_random_directed_graph(num_nodes, num_edges, scc_ratio, num_scc)

# 可视化图并保存为文件
# visualize_graph(G, graph_image_filename)

# 保存为txt文件
save_graph_as_edge_list(G, edges_filename)
