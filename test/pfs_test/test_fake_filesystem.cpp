#include <catch2/catch_test_macros.hpp>
#include <pfs/fake_filesystem.hpp>
#include <set>

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
    REQUIRE(fs.create_directories("four"));
    REQUIRE(fs.exists("three"));
    REQUIRE(fs.exists("four"));
    REQUIRE(fs.is_directory("three"));
    REQUIRE(fs.is_directory("four"));
    REQUIRE_THROWS_AS(fs.current_path("does/not/exist"),
                      std::filesystem::filesystem_error);
  }

  SECTION("special directories . and ..") {
    REQUIRE(fs.create_directories(root / "one/two/three"));
    REQUIRE_NOTHROW(fs.current_path("one/two"));
    REQUIRE(fs.current_path() == root / "one/two");
    REQUIRE(fs.is_directory("."));     // '/one/two'
    REQUIRE(fs.is_directory(".."));    // '/one'
    REQUIRE(fs.is_directory("../..")); // '/'
    REQUIRE(fs.create_directories("../newdir/foo"));
    REQUIRE(fs.is_directory(root / "one/newdir/foo"));
  }

  SECTION("remove") {
    REQUIRE_THROWS_AS(fs.remove("."), pfs::filesystem_error);
    REQUIRE_THROWS_AS(fs.remove(".."), pfs::filesystem_error);
    REQUIRE(fs.create_directories("one/two/three"));
    REQUIRE(!fs.remove("one/two/three/four")); // nothing to remove
    REQUIRE_THROWS_AS(fs.remove("one"), pfs::filesystem_error); // not empty
    REQUIRE(fs.remove("one/two/three"));
    REQUIRE(!fs.exists("one/two/three"));
    REQUIRE(fs.remove("one/two"));
    REQUIRE(!fs.exists("one/two"));
    REQUIRE(fs.remove("one"));
    REQUIRE(!fs.exists("one"));
  }

  SECTION("remove_all") {
    REQUIRE_THROWS(fs.remove_all("."));
    REQUIRE_THROWS(fs.remove_all(".."));
    REQUIRE(fs.create_directories("one/two/three"));
    REQUIRE(fs.remove_all("one") == 3);
    REQUIRE(!fs.exists("one"));
  }

  SECTION("absolute") {
    REQUIRE(fs.absolute(".") == root);
    REQUIRE(fs.create_directories("one/two/three"));
    REQUIRE_NOTHROW(fs.current_path("one/two"));
    REQUIRE(fs.absolute("..") == (root / "one"));
  }

  SECTION("rename") {
    REQUIRE(fs.create_directories("a/b/c"));
    REQUIRE_NOTHROW(fs.rename("a/b/c", "a/foo"));
    REQUIRE(fs.is_directory("a/foo"));
    REQUIRE(!fs.is_directory("a/b/c"));
  }

  SECTION("directory_iterator") {
    REQUIRE(fs.create_directories("a"));
    REQUIRE(fs.create_directories("b"));
    REQUIRE(fs.create_directories("c"));
    std::set<pfs::path> expected{"a", "b", "c"};
    std::set<pfs::path> actual;
    for (auto it = fs.directory_iterator("."); !it->at_end(); it->increment()) {
      actual.insert(it->path().filename());
    }
    REQUIRE(actual == expected);
  }

  SECTION("recursive_directory_iterator") {
    REQUIRE(fs.create_directories("a/b/c"));
    REQUIRE(fs.create_directories("x/y/z"));
    REQUIRE(fs.create_directories("a/b/i"));
    std::set<pfs::path> expected{"./a", "./a/b", "./a/b/c", "./a/b/i",
                                 "./x", "./x/y", "./x/y/z"};
    std::set<pfs::path> actual;
    for (auto it = fs.recursive_directory_iterator("."); !it->at_end();
         it->increment()) {
      actual.insert(it->path());
    }
    REQUIRE(actual == expected);
  }
}