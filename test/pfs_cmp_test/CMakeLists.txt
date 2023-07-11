add_executable(pfs_cmp_test pfs_cmp_test.cpp)
find_package(GTest REQUIRED)
target_link_libraries(pfs_cmp_test PRIVATE GTest::gtest_main pfs)
