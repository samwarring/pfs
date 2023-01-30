#include <catch2/catch_test_macros.hpp>
#include <pfs/fake_filesystem.hpp>

TEST_CASE("fake_filesystem") {
  pfs::fake_filesystem fs;
  REQUIRE(fs.create_root(""));

  SECTION("root directory") {
    auto status = fs.status("/");
    REQUIRE(status.type() == pfs::file_type::directory);
  }
}