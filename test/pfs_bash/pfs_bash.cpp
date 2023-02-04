#include <iostream>
#include <pfs/fake_filesystem.hpp>
#include <pfs/std_filesystem.hpp>
#include <sstream>
#include <string>

std::ostream &operator<<(std::ostream &out, pfs::file_type t) {
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

class application {
private:
  pfs::std_filesystem real_fs_;
  pfs::fake_filesystem fake_fs_;
  pfs::filesystem *fs_{&fake_fs_};

  static void print_help() {
    std::cout << '\n'
              << "Available commands:\n"
              << '\n'
              << "  help           Print this message.\n"
              << "  real           Switch to real filesystem.\n"
              << "  fake           Switch to fake filesystem.\n"
              << "  pwd            Print working directory.\n"
              << "  cd DIR         Change working directory.\n"
              << "  mkdir DIR      Create new directory. Parent must exist.\n"
              << "  mkdirs DIR     Create directory and subdirectories.\n"
              << "  abs PATH       Convert to absolute path.\n"
              << "  stat PATH      Prints properties file or directory.\n"
              << "  exist PATH     Checks if the path exists.\n"
              << "  isdir PATH     Checks if the path is a directory.\n"
              << "  exit           Exit this program.\n"
              << std::endl;
  }

  void print_prompt() {
    if (fs_ == &real_fs_) {
      std::cout << "[real] ";
    } else {
      std::cout << "[fake] ";
    }
    std::cout << '[' << fs_->current_path().string() << "] ?> ";
    std::cout.flush();
  }

  static std::vector<std::string> read_command() {
    std::string line;
    std::getline(std::cin, line);

    enum { IN_WHITESPACE, IN_QUOTED, IN_UNQUOTED } state = IN_WHITESPACE;

    std::vector<std::string> tokens;
    for (auto ch : line) {
      switch (state) {
      case IN_WHITESPACE:
        if (ch == '\"') {
          tokens.emplace_back();
          state = IN_QUOTED;
        } else if (!std::isspace(ch)) {
          tokens.emplace_back();
          tokens.back() += ch;
          state = IN_UNQUOTED;
        }
        break;
      case IN_QUOTED:
        if (ch == '\"') {
          state = IN_WHITESPACE;
        } else {
          tokens.back() += ch;
        }
        break;
      case IN_UNQUOTED:
        if (std::isspace(ch)) {
          state = IN_WHITESPACE;
        } else {
          tokens.back() += ch;
        }
        break;
      default:
        break;
      }
    }

    return tokens;
  }

  static bool parsed(const std::vector<std::string> &tokens,
                     const char *command_name) {
    return tokens[0] == command_name;
  }

  static bool parsed(const std::vector<std::string> &tokens,
                     const char *command_name, const char *metavar) {
    if (tokens[0] == command_name) {
      if (tokens.size() > 1) {
        return true;
      }
      std::cout << "`" << command_name << "` missing required " << metavar
                << ". See `help`." << std::endl;
    }
    return false;
  }

public:
  void run() {
    std::cout << std::boolalpha;
    print_help();
    std::string line;
    for (;;) {
      try {
        print_prompt();
        auto tokens = read_command();
        if (tokens.empty()) {
          continue;

        } else if (parsed(tokens, "help")) {
          print_help();

        } else if (parsed(tokens, "exit")) {
          break;

        } else if (parsed(tokens, "real")) {
          fs_ = &real_fs_;

        } else if (parsed(tokens, "fake")) {
          fs_ = &fake_fs_;

        } else if (parsed(tokens, "pwd")) {
          std::cout << fs_->current_path().string() << std::endl;

        } else if (parsed(tokens, "cd", "DIR")) {
          fs_->current_path(tokens[1]);

        } else if (parsed(tokens, "mkdir", "DIR")) {
          std::cout << fs_->create_directory(tokens[1]) << std::endl;

        } else if (parsed(tokens, "mkdirs", "DIR")) {
          std::cout << fs_->create_directories(tokens[1]) << std::endl;

        } else if (parsed(tokens, "abs", "PATH")) {
          std::cout << fs_->absolute(tokens[1]) << std::endl;

        } else if (parsed(tokens, "stat", "PATH")) {
          auto status = fs_->status(tokens[1]);
          std::cout << "type: " << status.type() << '\n'
                    << "perms: " << status.permissions() << std::endl;

        } else if (parsed(tokens, "exist", "PATH")) {
          std::cout << fs_->exists(tokens[1]) << std::endl;

        } else if (parsed(tokens, "isdir", "PATH")) {
          std::cout << fs_->is_directory(tokens[1]) << std::endl;

        } else {
          std::cout << "Unrecognized command. Try running `help`." << std::endl;
        }
      } catch (const std::exception &e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
      }
    }
  }
};

int main() {
  application app;
  app.run();
}