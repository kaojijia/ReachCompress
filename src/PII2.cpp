#include <iostream>
#include "TGraph.h"
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <iterator>
#include <random>
#include<chrono>
#include<sstream>
#include<climits>
using namespace std;
int n, m;
TGraph graph;
bool query(int s, int t, vector<set<int>> &LIN, vector<set<int>> &LOUT)
{
    vector<int> result;

    set_intersection(LOUT[s].begin(), LOUT[s].end(), LIN[t].begin(), LIN[t].end(), insert_iterator<vector<int>>(result, result.begin()));
    return !result.empty();
}
void algorithm_1(vector<set<int>> &LIN, vector<set<int>> &LOUT, int v)
{
    // cout << "Vertex: " << v << endl;
    unordered_set<int> s;
    queue<int> q;
    q.push(v);
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        s.insert(u);
        if (query(v, u, LIN, LOUT))
        {
            continue;
        }
        LIN[u].insert(v);
        for (int j = graph.head[u]; j != -1; j = graph.next[j])
        {
            int t = graph.adjv[j];
            if (!s.count(t))
            {
                q.push(t);
            }
        }
    }
    unordered_set<int> s1;
    q.push(v);
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        s1.insert(u);
        if (query(u, v, LIN, LOUT))
        {
            continue;
        }
        LOUT[u].insert(v);
        for (int j = graph.rhead[u]; j != -1; j = graph.rnext[j])
        {
            int t = graph.radjv[j];
            if (!s1.count(t))
            {
                q.push(t);
            }
        }
    }
}
void algorithm_2(vector<set<int>> &LIN, vector<set<int>> &LOUT, vector<int> &node)
{
    for (int i = 0; i < n; i++)
    {
        algorithm_1(LIN, LOUT, node[i]);
    }
}

// 以下为生成测试用例代码

// 生成numPoints个随机顶点
vector<int> generateRandomStartPoints(int minVal, int maxVal, int numPoints)
{
    vector<int> startPoints;
    unordered_set<int> usedPoints; // 用于去重

    // 初始化随机数生成器
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(minVal, maxVal);

    // 生成 numPoints 个不同的随机点
    while (startPoints.size() < numPoints)
    {
        int point = dis(gen);
        if (usedPoints.find(point) == usedPoints.end())
        { // 确保不重复
            startPoints.push_back(point);
            usedPoints.insert(point);
        }
    }

    return startPoints;
}

// 生成可达性点对
void TestReachablePairs(int numPairs, vector<int> &startPoints, vector<set<int>> &LIN, vector<set<int>> &LOUT)
{
    int pairCount = 0;

    for (auto s : startPoints)
    {
        vector<int> distance(n, INT_MAX);
        vector<int> parent(n, -1); // 用于记录每个节点的前驱节点
        queue<int> q;
        distance[s] = 0;
        q.push(s);

        int furthestNode = s; // 记录最远节点

        while (!q.empty())
        {
            int v = q.front();
            q.pop();

            for (int i = graph.head[v]; i != -1; i = graph.next[i])
            {
                int t = graph.adjv[i];
                if (distance[t] == INT_MAX)
                {
                    distance[t] = distance[v] + 1;
                    q.push(t);
                    parent[t] = v;
                    // 如果当前找到的距离比之前的最远距离更远，则更新最远节点
                    if (distance[t] > distance[furthestNode])
                    {
                        furthestNode = t;
                    }
                }
            }
        }

        // 如果找到的最远节点不是起点，记录点对 (s, furthestNode)
        if (furthestNode != s)
        {
            if (query(s, furthestNode, LIN, LOUT))
            {
                pairCount++;
            }
            else
            {
                cout << "Error: query(" << s << ", " << furthestNode << ") failed!" << endl;
                cout << "Path from " << s << " to " << furthestNode << " (length: " << distance[furthestNode] << "): ";
                vector<int> path;
                for (int v = furthestNode; v != -1; v = parent[v])
                {
                    path.push_back(v);
                }
                reverse(path.begin(), path.end());
                for (int v : path)
                {
                    cout << v << " ";
                }
                cout << endl;
                exit(0);
            }

            if (pairCount >= numPairs)
            {
                cout << pairCount << endl;
                return;
            }
        }
    }
}

template <typename T>
size_t calculateSetMemory(const std::set<T>& s) {
	// 计算 set 的内存大小，假设每个元素是 int 类型
	return s.size() * sizeof(T) + sizeof(s);
}

size_t calculateVectorSetMemory(const std::vector<std::set<int>>& vec) {
	size_t totalSize = 0;
	for (const auto& s : vec) {
		totalSize += calculateSetMemory(s);
	}
	// 返回大小为字节数
	return totalSize;
}

double calculateLINLOUTMemoryMB(const std::vector<std::set<int>>& LIN, const std::vector<std::set<int>>& LOUT) {
	size_t LINSize = calculateVectorSetMemory(LIN);
	size_t LOUTSize = calculateVectorSetMemory(LOUT);

	// 计算总大小并转换为MB（1MB = 1024 * 1024字节）
	double totalSizeMB = static_cast<double>(LINSize + LOUTSize) / (1024 * 1024);

	return totalSizeMB;
}

int main()
{
    graph.readGraph("Email-EuAllDAg.txt");
    n = graph.n;
    m = graph.m;
    // graph.printGraph();
    vector<set<int>> LIN(n + 1);
    vector<set<int>> LOUT(n + 1);
	auto start1 = std::chrono::high_resolution_clock::now();
    vector<int> node = graph.getSortedVerticesByDegreeProduct(); // 顶点策略
	
    algorithm_2(LIN, LOUT, node);
 
    int minVal = 100;      // 最小值
    int maxVal = 15000;    // 最大值
    int numPoints = 10000; // 需要生成的起始点数量
	auto end1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> compressTime = end1 - start1;
	cout << compressTime.count() << endl;
	cout << calculateLINLOUTMemoryMB(LIN,LOUT);
	std::string line;
	std::ifstream inputFile("queryEmail.txt");
	if (!inputFile.is_open()) {
		std::cout << "Failed to open input file." << std::endl;
		return 1;
	}

	std::ofstream outputFile("result.txt");
	if (!outputFile.is_open()) {
		std::cout << "Failed to open output file." << std::endl;
		inputFile.close();
		return 1;
	}

	while (std::getline(inputFile, line)) {
		std::istringstream iss(line);

		int first, second;
		if (!(iss >> first >> second)) {
			continue;
		}
		auto start2 = std::chrono::high_resolution_clock::now();
		if (query(first, second, LIN, LOUT)){
			auto end2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> qTime = end2 - start2;
			outputFile << 1 << " " << qTime.count() << endl;
		}
		else {
			outputFile << 0 << " " << first << " " << second << endl;
		}

	}

    return 0;
}
