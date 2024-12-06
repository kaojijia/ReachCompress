cmake_minimum_required(VERSION 3.10)
project(ReachCompress)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-g)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# 添加src目录
add_subdirectory(src)
add_subdirectory(external/googletest)
# add_subdirectory(external/infomap)  

# 包含include目录和GoogleTest头文件
include_directories(${gtest_SOURCE_DIR}/include  ${gtest_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/infomap/src)


# 查找 pthread 库
find_package(Threads REQUIRED)

# 启用测试
enable_testing()
add_subdirectory(test)
