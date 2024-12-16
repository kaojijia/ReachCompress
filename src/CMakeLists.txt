# src/CMakeLists.txt

# 生成静态库 libreach_comp.a
add_library(reach_comp STATIC 
    graph.cpp 
    pll.cpp 
    compression.cpp 
    utils/input_handler.cpp 
    utils/output_handler.cpp 
    BidirectionalBFS.cpp
    CompressedSearch.cpp
    PartitionManager.cpp
    LouvainPartitioner.cpp
    ReachRatio.cpp
    ImportPartitioner.cpp
    CSR.cpp
    BiBFSCSR.cpp
    filters/BloomFilter.cpp
    filters/TreeCover.cpp
)

# 包含头文件目录
target_include_directories(reach_comp PUBLIC ${CMAKE_SOURCE_DIR}/include)

