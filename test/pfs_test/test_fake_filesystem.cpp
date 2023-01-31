#include <catch2/catch_test_macros.hpp>
#include <pfs/fake_filesystem.hpp>

TEST_CASE("fake_filesystem") {
  pfs::fake_filesystem fs;
  REQUIRE(fs.create_root(""));

  SECTION("status of nonexistent path") {
    REQUIRE(fs.status("/does/not/exist").type() == pfs::file_type::not_found);
  }

  SECTION("root directory") {
    REQUIRE(fs.status("/").type() == pfs::file_type::directory);
  }

  SECTION("create directory") {
    REQUIRE(fs.create_directory("/hello"));
    REQUIRE(!fs.create_directory("/hello"));
    REQUIRE(fs.status("/hello").type() == pfs::file_type::directory);
    REQUIRE(fs.create_directory("/hello/goodbye"));
    REQUIRE(!fs.create_directory("/hello/goodbye"));
    REQUIRE(fs.status("/hello/goodbye").type() == pfs::file_type::directory);
    REQUIRE(!fs.create_directory("/parent/path/does/not/exist"));

    std::error_code ec;
    REQUIRE(!fs.create_directory("", ec));
    REQUIRE(ec == std::errc::no_such_file_or_directory);
  }
}