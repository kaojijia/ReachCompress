cmake_minimum_required(VERSION 3.10)
project(PLL_Project)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加src目录
add_subdirectory(src)

# 添加test目录，启用测试
# enable_testing()
# add_subdirectory(test)
