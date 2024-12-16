#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <set>
#include <cstring>  // 使用 memset
#include <dirent.h> // 使用 dirent.h 替代 filesystem
#include <algorithm>
#include <map>
#include <unordered_map>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/resource.h>

using namespace std;

struct ReachableArray {
    int* data;
    int size;
    int capacity;

    ReachableArray() : data(nullptr), size(0), capacity(0) {}

    ~ReachableArray() {
        delete[] data;
    }




    void add(int value) {
        if (!exists(value)) {
            if (size == capacity) {
                capacity = (capacity == 0) ? 2 : capacity * 2;
                int* newData = new int[capacity];
                memcpy(newData, data, size * sizeof(int));
                delete[] data;
                data = newData;
            }
            data[size++] = value;
        }
    }

    bool exists(int value) const {
        for (int i = 0; i < size; ++i) {
            if (data[i] == value) {
                return true;
            }
        }
        return false;
    }
};

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    tm local_tm;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&local_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &local_tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(6) << std::setfill('0') << now_us.count();
    return ss.str();
}

void loadGraph(const string& filename, vector<vector<int>>& adjList, int& numNodes) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "无法打开文件: " << filename << endl;
        return;
    }

    set<int> uniqueNodes; // 存储所有有入度或出度的节点
    vector<pair<int, int>> edges;
    string line;
    while (getline(file, line)) {
        int source, target;
        sscanf(line.c_str(), "%d %d", &source, &target);
        edges.emplace_back(source, target);
        uniqueNodes.insert(source);
        uniqueNodes.insert(target);
    }
    file.close();

    // 有效节点数为 uniqueNodes 的大小
    numNodes = uniqueNodes.size();

    // 创建邻接表
    adjList.resize(numNodes);
    map<int, int> nodeMapping;
    int index = 0;
    for (int node : uniqueNodes) {
        nodeMapping[node] = index++;
    }

    for (const auto& edge : edges) {
        int mappedSource = nodeMapping[edge.first];
        int mappedTarget = nodeMapping[edge.second];
        adjList[mappedSource].push_back(mappedTarget);
    }
}

bool topologicalSort(const vector<vector<int>>& adjList, vector<int>& topoOrder) {
    int n = adjList.size();
    vector<int> inDegree(n, 0);

    for (const auto& neighbors : adjList) {
        for (int neighbor : neighbors) {
            ++inDegree[neighbor];
        }
    }

    queue<int> zeroInDegree;
    for (int i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            zeroInDegree.push(i);
        }
    }

    while (!zeroInDegree.empty()) {
        int node = zeroInDegree.front();
        zeroInDegree.pop();
        topoOrder.push_back(node);

        for (int neighbor : adjList[node]) {
            if (--inDegree[neighbor] == 0) {
                zeroInDegree.push(neighbor);
            }
        }
    }

    return topoOrder.size() == n;
}

double computeReachRatio(const vector<vector<int>>& adjList, const vector<int>& topoOrder) {
    int n = adjList.size();
    vector<ReachableArray> reachable(n);

    for (auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it) {
        int node = *it;
        for (int neighbor : adjList[node]) {
            for (int i = 0; i < reachable[neighbor].size; ++i) {
                reachable[node].add(reachable[neighbor].data[i]);
            }
            reachable[node].add(neighbor);
        }
        reachable[node].add(node);
    }

    long long reachablePairs = 0;
    for (const auto& arr : reachable) {
        reachablePairs += arr.size - 1;
    }

    long long totalPairs = static_cast<long long>(n) * (n - 1);
    return totalPairs > 0 ? static_cast<double>(reachablePairs) / totalPairs : 0.0;
}

bool hasCycle(const vector<vector<int>>& adjList) {
    vector<int> topoOrder;
    return !topologicalSort(adjList, topoOrder);
}

void processDirectory(const string& path, const string& outputFile) {
    ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        cerr << "无法打开输出文件: " << outputFile << endl;
        return;
    }

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        cerr << "无法打开目录: " << path << endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string fileName = entry->d_name;

        if (fileName == "." || fileName == "..") {
            continue;
        }

        string filePath = path + "/" + fileName;
        cout << "处理文件: " << filePath << endl;

        vector<vector<int>> adjList;
        int numNodes;
        loadGraph(filePath, adjList, numNodes);

        if (hasCycle(adjList)) {
            cout << "输入图包含环，跳过文件: " << filePath << endl;
            outFile << "文件: " << filePath << ", 错误: 输入图包含环\n";
        } else {
            cout <<getCurrentTimestamp()<< "  开始处理文件: " << filePath << endl;
            vector<int> topoOrder;
            topologicalSort(adjList, topoOrder);
            double reachRatio = computeReachRatio(adjList, topoOrder);
            cout <<getCurrentTimestamp()<< "    文件: " << filePath << ", 可达性比例: " << reachRatio << endl;
            outFile <<getCurrentTimestamp()<< "     文件: " << filePath << ", 可达性比例: " << reachRatio << "\n";
        }
    }

    closedir(dir);
    outFile.close();
}


int main() {
    string directoryPath = PROJECT_ROOT_DIR"/Edges/DAGs/large";
    string outputFile = PROJECT_ROOT_DIR"/ratio_results.txt";

    processDirectory(directoryPath, outputFile);

    return 0;
}