import os

def replace_tabs_with_spaces(file_path):
    with open(file_path, 'r', encoding='utf-8-sig') as file:
        content = file.read()
    
    # 替换制表符为空格
    content = content.replace('\t', ' ')
    
    with open(file_path, 'w', encoding='utf-8') as file:
        file.write(content)

def process_files_in_directory(directory):
    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path):
            replace_tabs_with_spaces(file_path)
            print(f"Processed {file_path}")

if __name__ == "__main__":
    directory = "Edges/medium"
    process_files_in_directory(directory)