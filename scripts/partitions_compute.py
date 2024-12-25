import matplotlib
matplotlib.use('Agg')  # 使用Agg后端
import matplotlib.pyplot as plt
from collections import Counter

# 读取TXT文件并统计第二列数字的出现次数
def read_and_count(file_path):
    counts = Counter()
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()  # 假设数据是用空格分隔的
            if len(parts) > 1:
                try:
                    num = int(parts[1])
                    if num != -1:
                        counts[num] += 1
                except ValueError:
                    continue
    return counts

# 绘制统计图
def plot_counts(counts):
    numbers = list(counts.keys())
    frequencies = list(counts.values())

    plt.bar(numbers, frequencies)
    plt.xlabel('分区号')
    plt.ylabel('包含点数')
    plt.title('每个分区包含点数统计')
    plt.savefig('output.png')  # 保存图表为PNG文件
    # plt.show()  # 注释掉显示图表的代码

if __name__ == "__main__":
    file_path = '/home/reco/Projects/ReachCompress/Partitions/livejournal_rr.txt'  # 请将此路径替换为你的TXT文件路径
    counts = read_and_count(file_path)
    plot_counts(counts)