import os
import csv

input_folder = './Edges/generate'  # 当前文件夹
output_folder = './converted_csv_files'  # 输出文件夹

def ensure_folder_exists(folder):
    """确保文件夹存在，如果不存在则创建它。"""
    if not os.path.exists(folder):
        os.makedirs(folder)

def convert_edges_to_csv(input_file, output_file):
    """将边集文件的分隔符改为逗号，并保存为CSV格式"""
    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', newline='', encoding='utf-8') as outfile:
        csv_writer = csv.writer(outfile)
        csv_writer.writerow(['Source', 'Target'])  # 写入CSV头
        
        for line in infile:
            line = line.strip()
            if line:  # 确保行不为空
                source, target = line.split()  # 假设空格作为分隔符
                csv_writer.writerow([source, target])

    print(f"边集文件已成功转换为CSV格式，保存到 {output_file}")

def convert_all_edge_files_in_folder(input_folder, output_folder):
    """转换当前文件夹内所有文件"""
    ensure_folder_exists(output_folder)
    
    for filename in os.listdir(input_folder):
        input_file = os.path.join(input_folder, filename)
        output_file = os.path.join(output_folder, f"{os.path.splitext(filename)[0]}.csv")
        convert_edges_to_csv(input_file, output_file)

# 执行转换
convert_all_edge_files_in_folder(input_folder, output_folder)
