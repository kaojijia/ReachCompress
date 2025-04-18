# src/CMakeLists.txt

# 生成静态库 libreach_comp.a
add_library(reach_comp STATIC 
    struct/graph.cpp 
    struct/CSR.cpp
    struct/PartitionManager.cpp

    search/BiBFSCSR.cpp
    search/BidirectionalBFS.cpp
    search/pll.cpp 
    search/CompressedSearch.cpp
    search/SetSearch.cpp

    partitioner/LouvainPartitioner.cpp
    partitioner/ReachRatioPartitioner.cpp
    partitioner/ImportPartitioner.cpp
    partitioner/RandomPartitioner.cpp
    partitioner/MultiCutPartitioner.cpp
    partitioner/LabelPropagationPartitioner.cpp
    partitioner/TraversePartitioner.cpp
    
    utils/ReachRatio.cpp
    utils/compression.cpp 
    utils/input_handler.cpp 
    utils/output_handler.cpp
    utils/cal_ratio.cpp 
    filters/BloomFilter.cpp
    filters/TreeCover.cpp
)

# 包含头文件目录
target_include_directories(reach_comp PUBLIC ${CMAKE_SOURCE_DIR}/include)
