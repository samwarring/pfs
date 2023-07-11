#include <gtest/gtest.h>
#include <pfs/fake_filesystem.hpp>
#include <pfs/std_filesystem.hpp>

// Each of these test cases perform a series of filesystem operations with the
// std_filesystem and fake_filesystem classes. The outputs of the operations and
// side-effects are compared against each other to verify they have the same
// behavior.

// Provide comparisons for other data types
namespace std {
namespace filesystem {
bool operator==(const file_status a, const file_status b) {
  return a.permissions() == b.permissions() && a.type() == b.type();
}
} // namespace filesystem
} // namespace std

// clang-format off
//
// These macros call the same API for both filesystem objects and compare their
// results. This is the macro naming convention:
// COMPARE_{R|V}{0|N}
//           |     +--> 0 = No arguments (excluding the error code argument)
//           |          N = At least 1 argument (excluding the error code argument)
//           +--------> R = compare return value
//                      V = void return type, do not compare
//
// clang-format on
#define COMPARE_R0(api)                                                        \
  {                                                                            \
    using ret_type = decltype(sfs.api());                                      \
    ret_type std_ret{}, fake_ret{};                                            \
    pfs::error_code std_ec{}, fake_ec{};                                       \
    std_ret = sfs.api(std_ec);                                                 \
    fake_ret = ffs.api(fake_ec);                                               \
    ASSERT_EQ(std_ret, fake_ret);                                              \
    ASSERT_EQ(std_ec, fake_ec);                                                \
  }

#define COMPARE_RN(api, ...)                                                   \
  {                                                                            \
    using ret_type = decltype(sfs.api(__VA_ARGS__));                           \
    ret_type std_ret{}, fake_ret{};                                            \
    pfs::error_code std_ec{}, fake_ec{};                                       \
    std_ret = sfs.api(__VA_ARGS__, std_ec);                                    \
    fake_ret = ffs.api(__VA_ARGS__, fake_ec);                                  \
    ASSERT_EQ(std_ret, fake_ret);                                              \
    ASSERT_EQ(std_ec, fake_ec);                                                \
  }

#define COMPARE_VN(api, ...)                                                   \
  {                                                                            \
    pfs::error_code std_ec{}, fake_ec{};                                       \
    sfs.api(__VA_ARGS__, std_ec);                                              \
    ffs.api(__VA_ARGS__, fake_ec);                                             \
    ASSERT_EQ(std_ec, fake_ec);                                                \
  }

struct ComparisonTest : public testing::Test {
  static inline pfs::path initial_cwd;

  // Save the cwd before any test cases get a chance to modify it.
  static void SetUpTestSuite() {
    pfs::std_filesystem fs;
    initial_cwd = fs.current_path();
  }

  // Restore initial cwd before moving to another test suite.
  static void TearDownTestSuite() {
    pfs::std_filesystem fs;
    fs.current_path(initial_cwd);
  }

  // Comparing behavior of these objects.
  pfs::std_filesystem sfs;
  pfs::fake_filesystem ffs;
  pfs::error_code sec; // sfs error code
  pfs::error_code fec; // ffs error code

  // Each test case needs an isolated directory of the real filesystem to
  // perform its operations. Make one named after the current test case.
  void SetUp() override {
    auto test_info = testing::UnitTest::GetInstance()->current_test_info();
    pfs::path test_dir =
        initial_cwd / test_info->test_suite_name() / test_info->name();
    if (sfs.exists(test_dir)) {
      sfs.remove_all(test_dir);
    }
    sfs.create_directories(test_dir);
    ffs.create_directories(test_dir);

    // Set the test_dir as the cwd during the test
    sfs.current_path(test_dir);
    ffs.current_path(test_dir);
  }
};

TEST_F(ComparisonTest, CurrentPath) { COMPARE_R0(current_path); }

TEST_F(ComparisonTest, CreateDirectoryParentExists) {
  COMPARE_RN(create_directory, "subdir");
  COMPARE_RN(is_directory, "subdir");
}

TEST_F(ComparisonTest, CreateDirectoryParentDoesNotExist) {
  COMPARE_RN(create_directory, "subdir/subdir2");
  COMPARE_RN(is_directory, "subdir");
  COMPARE_RN(is_directory, "subdir/subdir2");
}

TEST_F(ComparisonTest, CreateDirectoryInParent) {
  COMPARE_RN(create_directory, "subdir1");
  COMPARE_VN(current_path, "subdir1");
  COMPARE_RN(create_directory, "../subdir2");
  COMPARE_RN(is_directory, "../subdir1");
  COMPARE_RN(is_directory, "../subdir2");
}

TEST_F(ComparisonTest, DirectoryStatus) {
  COMPARE_RN(create_directory, "subdir");
  COMPARE_RN(status, "subdir");
}