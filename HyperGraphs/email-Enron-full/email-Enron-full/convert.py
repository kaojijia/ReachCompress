import os

# --- 配置 ---
# 输入文件路径 (根据您提供的附件信息)
input_simplices_file = '/root/ReachCompress/HyperGraphs/email-Enron-full/email-Enron-full/email-Enron-full-simplices.txt'

# 输出文件基础目录
output_base_dir = '/root/ReachCompress/HyperGraphs'
# --- 结束配置 ---

def convert_hypergraph_simplices(simplices_path, output_dir):
    """
    将超图的simplices文件转换为简单的边列表格式（每行包含一条超边的所有节点）。

    Args:
        simplices_path (str): 输入的simplices文件路径。
        output_dir (str): 输出文件的目标目录。
    """
    print(f"开始处理文件: {simplices_path}")

    # 检查输入文件是否存在
    if not os.path.exists(simplices_path):
        print(f"错误: 输入文件未找到于 {simplices_path}")
        return
    if not os.path.isfile(simplices_path):
        print(f"错误: 提供的路径不是一个文件 {simplices_path}")
        return

    try:
        # --- 确定数据集名称和输出路径 ---
        # 从输入路径推断数据集名称
        # 假设路径结构为 /.../dataset_name/dataset_name/...-simplices.txt
        # 我们需要 'dataset_name'
        containing_dir = os.path.dirname(simplices_path) # 获取文件所在目录 (.../email-Enron-full/email-Enron-full)
        parent_of_containing_dir = os.path.dirname(containing_dir) # 获取上一级目录 (.../email-Enron-full)
        dataset_name = os.path.basename(parent_of_containing_dir) # 获取目录名 (email-Enron-full)

        # 如果上面的方法失败（例如路径结构不同），尝试备用方法
        if not dataset_name or dataset_name == 'HyperGraphs': # 避免取到根目录名
             base_name = os.path.basename(simplices_path)
             # 尝试从文件名去除后缀
             dataset_name = base_name.replace('-simplices.txt', '')
             # 再次检查，如果还是无效，则使用默认名称
             if not dataset_name or dataset_name == base_name:
                 dataset_name = "converted_hypergraph"
                 print(f"警告: 无法从路径结构可靠地确定数据集名称。将使用 '{dataset_name}'。")

        # 构建输出文件名和完整路径
        output_filename = f"{dataset_name}.edgelist" # 使用 .edgelist 或 .txt 作为扩展名
        output_path = os.path.join(output_dir, output_filename)

        # 确保输出目录存在
        os.makedirs(output_dir, exist_ok=True)

        print(f"输出文件将保存至: {output_path}")

        # --- 处理文件 ---
        processed_edges = 0
        skipped_lines = 0
        with open(simplices_path, 'r') as infile, open(output_path, 'w') as outfile:
            # 跳过第一行（通常是注释或元数据）
            try:
                first_line = next(infile).strip()
                if first_line.startswith("//"):
                    print(f"已跳过注释行: {first_line}")
                else:
                    # 如果第一行不是注释，可能需要处理它，或者回退文件指针
                    # 这里我们假设第一行总是要跳过的
                    print(f"已跳过第一行（假设为元数据/注释）: {first_line}")
                    # 如果需要处理第一行，取消下一行的注释并调整逻辑
                    # infile.seek(0) # 回到文件开头
            except StopIteration:
                print("警告: 输入文件为空或只有一行。")
                return # 文件为空，无需处理

            # 逐行读取simplices文件
            for i, line in enumerate(infile, 1): # 从第二行开始计数 (索引+1)
                line = line.strip()
                if not line: # 跳过空行
                    skipped_lines += 1
                    continue

                parts = line.split()

                try:
                    # 验证格式：第一个数字是边的大小k，后面跟着k个节点ID
                    if not parts:
                        skipped_lines += 1
                        continue

                    size_k = int(parts[0])
                    node_ids_str = parts[1:]

                    # 检查节点数量是否与声明的大小匹配
                    if len(node_ids_str) != size_k:
                        print(f"警告: 第 {i+1} 行格式错误。声明大小 {size_k} 但找到 {len(node_ids_str)} 个节点。跳过行: '{line}'")
                        skipped_lines += 1
                        continue

                    # 检查节点ID是否都是数字（可选，但更健壮）
                    # node_ids_int = [int(node) for node in node_ids_str] # 如果需要整数验证

                    # 将节点ID列表（字符串形式）用空格连接
                    output_line = " ".join(node_ids_str)

                    # 写入输出文件
                    outfile.write(output_line + "\n")
                    processed_edges += 1

                except (ValueError, IndexError) as e:
                    print(f"警告: 无法解析第 {i+1} 行。错误: {e}。跳过行: '{line}'")
                    skipped_lines += 1
                    continue

        print(f"\n转换完成。")
        print(f"成功处理 {processed_edges} 条超边。")
        if skipped_lines > 0:
            print(f"跳过了 {skipped_lines} 行（空行或格式错误）。")

    except FileNotFoundError:
        # 这个检查在函数开始时已经做过，但为了完整性保留
        print(f"错误: 输入文件未找到于 {simplices_path}")
    except IOError as e:
        print(f"错误: 读写文件时发生IO错误: {e}")
    except Exception as e:
        print(f"发生意外错误: {e}")

# --- 执行转换 ---
if __name__ == "__main__":
    convert_hypergraph_simplices(input_simplices_file, output_base_dir)