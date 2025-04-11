#ifndef SIMPLEX_READER_H
#define SIMPLEX_READER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "Hypergraph.h"

/**
 * 用于处理Cornell单纯形数据集的工具类
 */
class SimplexReader {
public:
    /**
     * 从Cornell格式的时态单纯形数据集转换为超图格式文件
     * 
     * @param nVertsPath 顶点数量文件路径 (dataset-nverts.txt)
     * @param simplicesPath 单纯形顶点文件路径 (dataset-simplices.txt)
     * @param outputPath 输出超图文件路径
     */
    static void convertSimplexToHypergraph(
        const std::string &nVertsPath, 
        const std::string &simplicesPath, 
        const std::string &outputPath)
    {
        // 打开输入文件
        std::ifstream nVertsFile(nVertsPath);
        std::ifstream simplicesFile(simplicesPath);
        
        if (!nVertsFile.is_open()) {
            throw std::runtime_error("Cannot open nverts file: " + nVertsPath);
        }
        if (!simplicesFile.is_open()) {
            throw std::runtime_error("Cannot open simplices file: " + simplicesPath);
        }
        
        // 打开输出文件
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open()) {
            throw std::runtime_error("Cannot open output file: " + outputPath);
        }
        
        std::string line;
        int nVerts;
        
        // 逐行读取单纯形顶点数并处理
        while (std::getline(nVertsFile, line)) {
            // 获取当前单纯形的顶点数
            nVerts = std::stoi(line);
            
            // 一行输出表示一个超边（单纯形）
            for (int i = 0; i < nVerts; ++i) {
                if (std::getline(simplicesFile, line)) {
                    // 第一个顶点前不加空格，其他顶点前加空格
                    if (i > 0) {
                        outputFile << " ";
                    }
                    outputFile << line;
                } else {
                    throw std::runtime_error("Simplices file ended unexpectedly.");
                }
            }
            // 每个超边结束后换行
            outputFile << std::endl;
        }
        
        // 关闭文件
        nVertsFile.close();
        simplicesFile.close();
        outputFile.close();
        
        std::cout << "Conversion completed. Hypergraph saved to: " << outputPath << std::endl;
    }

    /**
     * 从转换后的超图格式文件读取超图
     * 
     * @param filename 超图文件路径
     * @return 构建的Hypergraph对象
     */
    static Hypergraph fromHypergraphFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::string line;
        int max_vertex_id = -1;
        std::vector<std::vector<int>> edge_data;

        // 第一遍扫描，找出最大顶点ID并收集边数据
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            int vertex_id;
            std::vector<int> edge;

            while (iss >> vertex_id) {
                edge.push_back(vertex_id);
                max_vertex_id = std::max(max_vertex_id, vertex_id);
            }

            if (!edge.empty()) {
                edge_data.push_back(std::move(edge));
            }
        }

        // 创建超图并添加顶点和边
        Hypergraph hg(max_vertex_id + 1, edge_data.size());
        
        for (const auto &edge : edge_data) {
            hg.addHyperedge(edge);
        }

        return hg;
    }
};

#endif // SIMPLEX_READER_H