# 生成静态库libpll.a
add_library(pll STATIC graph.cpp pll.cpp graph_pruner.cpp compression.cpp input_handler.cpp output_handler.cpp BidirectionalBFS.cpp)

# 包含头文件目录
target_include_directories(pll PUBLIC ../include)

# 生成可执行文件
add_executable(pll_executable main.cpp)

# 链接静态库
target_link_libraries(pll_executable pll)
