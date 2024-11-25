import re

# 读取内容
content = """

"""
# 替换LaTeX格式
content = re.sub(r'\\\(\s*(.*?)\s*\\\)', r'$\1$', content)

print(content)
