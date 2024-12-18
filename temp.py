# 打开文件并读取内容
with open("./dbpedia", "r") as file:  # 将 "your_file.txt" 替换为你的文件路径
    lines = file.readlines()

# 初始化一个列表用于存储边集
edge_list = []

# 遍历每一行，提取边信息
for line in lines:
    # 检查行是否以 'a ' 开头
    if not line.startswith("%"):
        # 分割行内容，提取边的起点和终点
        parts = line.split()
        start_node = int(parts[0])  # 边的起点
        end_node = int(parts[1])    # 边的终点
        edge_list.append((start_node, end_node))  # 将起点和终点作为元组存储



# 如果需要将边集写入一个新文件
with open("./dbpedia_edgelist", "w") as output_file:
    for edge in edge_list:
        output_file.write(f"{edge[0]} {edge[1]}\n")
