#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <filesystem>
#include <iostream>
#include <vector>

std::ostream &operator<<(std::ostream &out,
                         const std::filesystem::filesystem_error &e) {
  out << "filesystem_error:\n"
      << "  what: " << e.what() << '\n'
      << "  code: " << e.code() << '\n'
      << "  code.cat.name: " << e.code().category().name() << '\n'
      << "  code.msg: " << e.code().message();
  return out;
}

std::ostream &operator<<(std::ostream &out, std::filesystem::file_type t) {
  using ft = std::filesystem::file_type;
  switch (t) {
  case ft::none:
    return out << "none";
  case ft::not_found:
    return out << "not_found";
  case ft::regular:
    return out << "regular";
  case ft::directory:
    return out << "directory";
  case ft::symlink:
    return out << "symlink";
  case ft::block:
    return out << "block";
  case ft::character:
    return out << "character";
  case ft::fifo:
    return out << "fifo";
  case ft::socket:
    return out << "socket";
  case ft::unknown:
    return out << "unkown";
  default:
    return out << "default";
  }
}

std::ostream &operator<<(std::ostream &out, std::filesystem::perms p) {
  // credit: https://en.cppreference.com/w/cpp/filesystem/perms
  using std::filesystem::perms;
  auto show = [&](char op, perms perm) {
    out << (perms::none == (perm & p) ? '-' : op);
  };
  show('r', perms::owner_read);
  show('w', perms::owner_write);
  show('x', perms::owner_exec);
  show('r', perms::group_read);
  show('w', perms::group_write);
  show('x', perms::group_exec);
  show('r', perms::others_read);
  show('w', perms::others_write);
  show('x', perms::others_exec);
  return out;
}

std::ostream &operator<<(std::ostream &out, std::filesystem::file_status s) {
  out << "file_status:\n"
      << "type: " << s.type() << '\n'
      << "permissions: " << s.permissions();
  return out;
}

class subcommand {
protected:
  CLI::App *parser;

public:
  subcommand(CLI::App &parent_parser, std::string name) {
    parser = parent_parser.add_subcommand(name);
    parser->allow_windows_style_options(false);
  }

  virtual ~subcommand() {}

  virtual void run() = 0;

  void run_if_parsed() {
    if (parser->parsed()) {
      run();
    }
  }
};

struct subcommand_path : public subcommand {
  std::string path;

  subcommand_path(CLI::App &app) : subcommand(app, "path") {
    parser->add_option("path", path)
        ->description("Examine the argument as an std::path");
  }

  void run() override {
    std::filesystem::path p(path);
    std::cout << "\nroot_name: " << p.root_name() << '\n'
              << "root_directory: " << p.root_directory() << '\n'
              << "relative_path: " << p.relative_path() << '\n'
              << "stem: " << p.stem() << '\n'
              << "extension: " << p.extension() << '\n'
              << "iteration: ";
    for (auto part : p) {
      std::cout << part << ' ';
    }
    std::cout << '\n';
  }
};

struct subcommand_current_path : public subcommand {
  std::string set_path;

  subcommand_current_path(CLI::App &app) : subcommand(app, "current_path") {
    parser->add_option("--set", set_path)
        ->option_text("PATH")
        ->description(
            "Additionally, try to set current directory to this path.");
  }

  void run() override {
    std::cout << "\ncurrent_path(): " << std::filesystem::current_path()
              << "\n";
    if (parser->count("--set")) {
      std::cout << "\ncurrent_path(\"" << set_path << "\"): ";
      try {
        std::filesystem::current_path(set_path);
        std::cout << "ok\n";
      } catch (const std::filesystem::filesystem_error &e) {
        std::cout << e << '\n';
      }
    }
  }
};

struct subcommand_create_directory : public subcommand {
  std::string dir_path;

  subcommand_create_directory(CLI::App &app)
      : subcommand(app, "create_directory") {
    parser->add_option("path", dir_path)
        ->description("Attempt to create a directory at this path.");
  }

  void run() override {
    std::cout << "\ncreate_directory(\"" << dir_path << "\"): ";
    try {
      std::cout << std::filesystem::create_directory(dir_path) << '\n';
    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << e << '\n';
      std::cout << "  compare to errc::no_such_file_or_directory: "
                << (e.code() == std::errc::no_such_file_or_directory) << '\n';
    }
  }
};

struct subcommand_create_directories : public subcommand {
  std::string dir_path;

  subcommand_create_directories(CLI::App &app)
      : subcommand(app, "create_directories") {
    parser->add_option("path", dir_path)
        ->description("Attempt to create a directory at this path. Create all "
                      "directories along the path that do not exist.");
  }

  void run() override {
    std::cout << "\ncreate_directories(\"" << dir_path << "\"): ";
    try {
      std::cout << std::filesystem::create_directories(dir_path) << '\n';
    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << e << '\n';
    }
  }
};

struct subcommand_exists : public subcommand {
  std::string path;

  subcommand_exists(CLI::App &app) : subcommand(app, "exists") {
    parser->add_option("path", path)
        ->description("Checks if the provided path exists");
  }

  void run() override {
    std::cout << "\nexists(\"" << path << "\"): ";
    try {
      std::cout << std::filesystem::exists(path) << '\n';
    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << e << '\n';
    }
  }
};

struct subcommand_is_directory : public subcommand {
  std::string path;

  subcommand_is_directory(CLI::App &app) : subcommand(app, "is_directory") {
    parser->add_option("path", path)
        ->description("Checks if the provided path is a directory");
  }

  void run() override {
    std::cout << "\nis_directory(\"" << path << "\"): ";
    try {
      std::cout << std::filesystem::is_directory(path) << '\n';
    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << e << '\n';
    }
  }
};

struct subcommand_status : public subcommand {
  std::string path;

  subcommand_status(CLI::App &app) : subcommand(app, "status") {
    parser->add_option("path", path)
        ->description(
            "Gets the status of the file/directory at the given path.");
  }

  void run() override {
    std::cout << "\nstatus(\"" << path << "\"): ";
    try {
      std::cout << std::filesystem::status(path) << '\n';
    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << e << '\n';
    }
  }
};

int main(int argc, char **argv) {

  CLI::App app{"Peforms arbitrary std::filesystem operations, so their "
               "behavior can be replicated in pfs."};

  std::unique_ptr<subcommand> subcommands[]{
      std::make_unique<subcommand_path>(app),
      std::make_unique<subcommand_current_path>(app),
      std::make_unique<subcommand_create_directory>(app),
      std::make_unique<subcommand_create_directories>(app),
      std::make_unique<subcommand_exists>(app),
      std::make_unique<subcommand_is_directory>(app),
      std::make_unique<subcommand_status>(app)};

  CLI11_PARSE(app, argc, argv);

  for (auto &s : subcommands) {
    s->run_if_parsed();
  }
  std::cout << '\n';
}