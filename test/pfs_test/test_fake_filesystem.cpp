#include <catch2/catch_test_macros.hpp>
#include <pfs/fake_filesystem.hpp>

TEST_CASE("fake_filesystem") {
  pfs::fake_filesystem fs;
  auto root = fs.default_root();

  SECTION("status of nonexistent path") {
    REQUIRE(fs.status(root / "does/not/exist").type() ==
            pfs::file_type::not_found);
  }

  SECTION("root directory") {
    REQUIRE(fs.status(root).type() == pfs::file_type::directory);
  }

  SECTION("create directory") {
    REQUIRE(fs.create_directory(root / "hello"));
    REQUIRE(!fs.create_directory(root / "hello"));
    REQUIRE(fs.status(root / "hello").type() == pfs::file_type::directory);
    REQUIRE(fs.create_directory(root / "hello/goodbye"));
    REQUIRE(!fs.create_directory(root / "hello/goodbye"));
    REQUIRE(fs.status(root / "hello/goodbye").type() ==
            pfs::file_type::directory);

    std::error_code ec;
    REQUIRE(!fs.create_directory("", ec));
    REQUIRE(ec == std::errc::no_such_file_or_directory);
    REQUIRE(!fs.create_directory(root / "parent/path/does/not/exist", ec));
    REQUIRE(ec == std::errc::no_such_file_or_directory);
  }

  SECTION("create_directories") {
    REQUIRE(fs.create_directories(root / "you/say/goodbye/i/say/hello"));
    REQUIRE(!fs.create_directories(root / "you/say/goodbye/i/say/hello"));

    std::error_code ec;
    REQUIRE(!fs.create_directories("", ec));
    REQUIRE(ec == std::errc::no_such_file_or_directory);
  }

  SECTION("exists") {
    REQUIRE(!fs.exists(root / "let"));
    REQUIRE(!fs.exists(root / "let/it"));
    REQUIRE(!fs.exists(root / "let/it/be"));
    REQUIRE(fs.create_directories(root / "let/it/be"));
    REQUIRE(fs.exists(root / "let"));
    REQUIRE(fs.exists(root / "let/it"));
    REQUIRE(fs.exists(root / "let/it/be"));
  }

  SECTION("is_directory") {
    REQUIRE(!fs.is_directory(root / "hey"));
    REQUIRE(!fs.is_directory(root / "hey/jude"));
    REQUIRE(fs.create_directories(root / "hey/jude"));
    REQUIRE(fs.is_directory(root / "hey"));
    REQUIRE(fs.is_directory(root / "hey/jude"));
  }

  SECTION("current_path") {
    REQUIRE(fs.create_directories(root / "one/two/three"));
    REQUIRE_NOTHROW(fs.current_path(root / "one/two"));
    REQUIRE(fs.exists("three"));
    REQUIRE(fs.is_directory("three"));
  }
}