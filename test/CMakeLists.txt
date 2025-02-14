

# 如果您使用 add_subdirectory(external/googletest) 来包含 GoogleTest，不需要再使用 find_package(GTest REQUIRED)。两者同时使用可能导致冲突。


# 编译测试
#add_executable(test_graph test_graph.cpp)
#target_link_libraries(test_graph reach_comp gtest gtest_main)

# add_executable(test_pll test_pll.cpp)
# target_link_libraries(test_pll reach_comp gtest gtest_main)

# add_executable(test_bi_bfs test_bi_bfs.cpp)
# target_link_libraries(test_bi_bfs reach_comp gtest gtest_main)

# add_executable(test_comp test_comp.cpp)
# target_link_libraries(test_comp reach_comp gtest gtest_main)

# add_executable(test_part test_part.cpp)
# target_link_libraries(test_part reach_comp gtest gtest_main)

# add_executable(test_part_reach test_part_reach.cpp)
# target_link_libraries(test_part_reach reach_comp gtest gtest_main)


# add_executable(test_ratio test_ratio.cpp)
# target_link_libraries(test_ratio reach_comp gtest gtest_main)


# add_executable(test_CSR test_CSR.cpp)
# target_link_libraries(test_CSR reach_comp gtest gtest_main)

# add_executable(test_Tree_Cover test_tree_cover.cpp)
# target_link_libraries(test_Tree_Cover reach_comp gtest gtest_main)

# add_executable(test_Bloom test_bloom.cpp)
# target_link_libraries(test_Bloom reach_comp gtest gtest_main)


add_executable(test_Set test_set_reach.cpp)
target_link_libraries(test_Set reach_comp gtest gtest_main)

# add_executable(tes1t test.cpp)
# target_link_libraries(tes1t gtest gtest_main)

set_target_properties(${TARGET} PROPERTIES
COMPILE_FLAGS "-pthread"
LINK_FLAGS "-pthread")


# 添加测试到CTest框架
add_test(NAME TestGraph COMMAND test_graph)
# add_test(NAME TestPLL COMMAND test_pll)
# add_test(NAME TestBiBFS COMMAND test_bi_bfs)
# add_test(NAME TestComp COMMAND test_comp)

enable_testing()