#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <variant>
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
      << "  type: " << s.type() << '\n'
      << "  permissions: " << s.permissions();
  return out;
}

std::string examine_path(const std::filesystem::path &p) {
  std::ostringstream out;
  out << "path:\n"
      << "  root_name: " << p.root_name() << '\n'
      << "  root_directory: " << p.root_directory() << '\n'
      << "  relative_path: " << p.relative_path() << '\n'
      << "  stem: " << p.stem() << '\n'
      << "  extension: " << p.extension() << '\n'
      << "  iteration: ";
  for (auto part : p) {
    out << part << ' ';
  }
  return out.str();
}

class application {
private:
  typedef void (*path_fn_void)(const std::filesystem::path &);
  typedef bool (*path_fn_bool)(const std::filesystem::path &);
  typedef std::uintmax_t (*path_fn_uintmax)(const std::filesystem::path &);
  typedef std::string (*path_fn_string)(const std::filesystem::path &);
  typedef std::filesystem::file_status (*path_fn_status)(
      const std::filesystem::path &);
  using path_fn_any = std::variant<path_fn_void, path_fn_bool, path_fn_uintmax,
                                   path_fn_string, path_fn_status>;

  CLI::App app_{"Peforms arbitrary std::filesystem operations, so their "
                "behavior can be replicated in pfs."};

  std::string path_;

  struct subcommand {
    CLI::App *parser;
    path_fn_any path_fn;
  };

  std::vector<subcommand> subs_;

  template <typename Fn> void add_subcommand(const char *name, Fn path_fn) {
    subcommand sub;
    sub.parser = app_.add_subcommand(name);
    sub.parser->allow_windows_style_options(false);
    sub.parser->add_option("path", path_);
    sub.path_fn = path_fn;
    subs_.push_back(sub);
  }

  void add_subcommand_void(const char *name, path_fn_void path_fn) {
    add_subcommand(name, path_fn);
  }

  void add_subcommand_bool(const char *name, path_fn_bool path_fn) {
    add_subcommand(name, path_fn);
  }

  void add_subcommand_uintmax(const char *name, path_fn_uintmax path_fn) {
    add_subcommand(name, path_fn);
  }

  void add_subcommand_string(const char *name, path_fn_string path_fn) {
    add_subcommand(name, path_fn);
  }

  void add_subcommand_status(const char *name, path_fn_status path_fn) {
    add_subcommand(name, path_fn);
  }

public:
  int main(int argc, const char **argv) {
    try {
      app_.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
      return app_.exit(e);
    }
    for (auto &sub : subs_) {
      if (sub.parser->parsed()) {
        std::cout << sub.parser->get_name() << "(\"" << path_ << "\"): ";
        try {

          if (std::holds_alternative<path_fn_void>(sub.path_fn)) {
            std::get<path_fn_void>(sub.path_fn)(path_);
            std::cout << "ok\n";

          } else if (std::holds_alternative<path_fn_bool>(sub.path_fn)) {
            std::cout << std::get<path_fn_bool>(sub.path_fn)(path_) << '\n';
          }

          else if (std::holds_alternative<path_fn_uintmax>(sub.path_fn)) {
            std::cout << std::get<path_fn_uintmax>(sub.path_fn)(path_) << '\n';
          }

          else if (std::holds_alternative<path_fn_string>(sub.path_fn)) {
            std::cout << std::get<path_fn_string>(sub.path_fn)(path_) << '\n';
          }

          else if (std::holds_alternative<path_fn_status>(sub.path_fn)) {
            std::cout << std::get<path_fn_status>(sub.path_fn)(path_) << '\n';
          }

        } catch (const std::filesystem::filesystem_error &e) {
          std::cout << e;
        }
      }
    }
    return 0;
  }

  application() {
    add_subcommand_bool("exists", std::filesystem::exists);
    add_subcommand_bool("create_directory", std::filesystem::create_directory);
    add_subcommand_bool("create_directories",
                        std::filesystem::create_directories);
    add_subcommand_void("current_path", std::filesystem::current_path);
    add_subcommand_bool("is_directory", std::filesystem::is_directory);
    add_subcommand_string("path", examine_path);
    add_subcommand_bool("remove", std::filesystem::remove);
    add_subcommand_uintmax("remove_all", std::filesystem::remove_all);
    add_subcommand_status("status", std::filesystem::status);
  }
};

int main(int argc, const char **argv) {
  application app;
  return app.main(argc, argv);
}