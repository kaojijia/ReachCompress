# 查找GoogleTest库
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 编译测试
add_executable(test_graph test_graph.cpp)
target_link_libraries(test_graph reach_comp GTest::GTest GTest::Main)

add_executable(test_pll test_pll.cpp)
target_link_libraries(test_pll reach_comp GTest::GTest GTest::Main)

add_executable(test_bi_bfs test_bi_bfs.cpp)
target_link_libraries(test_bi_bfs reach_comp GTest::GTest GTest::Main)

add_executable(test_comp test_comp.cpp)
target_link_libraries(test_comp reach_comp GTest::GTest GTest::Main)



# 添加测试到CTest框架
add_test(NAME TestGraph COMMAND test_graph)
add_test(NAME TestPLL COMMAND test_pll)
add_test(NAME TestBiBFS COMMAND test_bi_bfs)
add_test(NAME TestComp COMMAND test_comp)

enable_testing()