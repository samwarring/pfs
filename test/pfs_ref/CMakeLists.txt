add_executable(pfs_ref pfs_ref.cpp)
find_package(CLI11 REQUIRED)
target_link_libraries(pfs_ref PRIVATE CLI11::CLI11)
target_compile_features(pfs_ref PRIVATE cxx_std_17)
