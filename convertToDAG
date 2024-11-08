import os
from collections import defaultdict

def read_edges(filename):
    edges = []
    with open(filename, 'r') as file:
        for line in file:
            u, v = map(int, line.strip().split())
            edges.append((u, v))
    return edges

def tarjan_scc(n, edges):
    index = 0
    stack = []
    indices = [-1] * n
    lowlink = [-1] * n
    on_stack = [False] * n
    sccs = []

    graph = defaultdict(list)
    for u, v in edges:
        graph[u].append(v)

    def strongconnect(v):
        nonlocal index
        call_stack = [(v, 0)]
        while call_stack:
            v, i = call_stack.pop()
            if i == 0:
                indices[v] = index
                lowlink[v] = index
                index += 1
                stack.append(v)
                on_stack[v] = True
                call_stack.append((v, 1))
                for w in graph[v]:
                    if indices[w] == -1:
                        call_stack.append((w, 0))
                    elif on_stack[w]:
                        lowlink[v] = min(lowlink[v], indices[w])
            else:
                if lowlink[v] == indices[v]:
                    scc = []
                    while True:
                        w = stack.pop()
                        on_stack[w] = False
                        scc.append(w)
                        if w == v:
                            break
                    sccs.append(scc)
                if stack:
                    w = stack[-1]
                    lowlink[w] = min(lowlink[w], lowlink[v])

    for v in range(n):
        if indices[v] == -1:
            strongconnect(v)

    return sccs

def compress_sccs(sccs, edges):
    scc_map = {}
    for i, scc in enumerate(sccs):
        for node in scc:
            scc_map[node] = i

    new_edges = set()
    for u, v in edges:
        if scc_map[u] != scc_map[v]:
            new_edges.add((scc_map[u], scc_map[v]))

    return new_edges

def write_edges(filename, edges):
    with open(filename, 'w') as file:
        for u, v in edges:
            file.write(f"{u} {v}\n")

def main():
    input_file = 'Edges/tweibo-edgelist.txt'
    output_file = 'Edges/tweibo-edgelist_DAG'

    edges = read_edges(input_file)
    nodes = set(u for u, v in edges).union(set(v for u, v in edges))
    n = max(nodes) + 1

    sccs = tarjan_scc(n, edges)
    new_edges = compress_sccs(sccs, edges)
    write_edges(output_file, new_edges)

if __name__ == "__main__":
    main()