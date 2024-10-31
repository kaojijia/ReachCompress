# 查找GoogleTest库
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 编译测试
add_executable(test_graph test_graph.cpp)
target_link_libraries(test_graph pll GTest::GTest GTest::Main)

add_executable(test_pll test_pll.cpp)
target_link_libraries(test_pll pll GTest::GTest GTest::Main)

add_executable(test_pruner test_pruner.cpp)
target_link_libraries(test_pruner pll GTest::GTest GTest::Main)

add_executable(test_bi_bfs test_bi_bfs.cpp)
target_link_libraries(test_bi_bfs pll GTest::GTest GTest::Main)

add_executable(test_comp test_comp.cpp)
target_link_libraries(test_comp pll GTest::GTest GTest::Main)


# add_executable(test_pll2 test_pll2.cpp)
# target_link_libraries(test_pll2 pll GTest::GTest GTest::Main)
# 添加测试到CTest框架
add_test(NAME TestGraph COMMAND test_graph)
add_test(NAME TestPLL COMMAND test_pll)
add_test(NAME TestPruner COMMAND test_pruner)
add_test(NAME TestBiBFS COMMAND test_bi_bfs)
add_test(NAME TestComp COMMAND test_comp)

enable_testing()