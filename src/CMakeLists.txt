# 生成静态库libpll.a
add_library(reach_comp STATIC graph.cpp pll.cpp compression.cpp utils/input_handler.cpp utils/output_handler.cpp BidirectionalBFS.cpp)

# 包含头文件目录
target_include_directories(reach_comp PUBLIC ../include)

# 生成可执行文件
add_executable(pll_executable main.cpp)

# 链接静态库
target_link_libraries(pll_executable reach_comp)
