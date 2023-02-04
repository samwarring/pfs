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

  void print_help() {
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

public:
  void run() {
    std::cout << std::boolalpha;
    print_help();
    std::string line;
    for (;;) {
      try {
        print_prompt();
        std::getline(std::cin, line);
        std::istringstream sin(line);
        std::string command;
        std::string arg;
        sin >> command >> arg;
        if (command.empty()) {
          continue;
        } else if (command == "help") {
          print_help();
        } else if (command == "exit") {
          break;
        } else if (command == "real") {
          fs_ = &real_fs_;
        } else if (command == "fake") {
          fs_ = &fake_fs_;
        } else if (command == "pwd") {
          std::cout << fs_->current_path().string() << std::endl;
        } else if (command == "cd") {
          if (arg.empty()) {
            std::cout << "`cd` missing required DIR. See `help`." << std::endl;
          } else {
            fs_->current_path(arg);
          }
        } else if (command == "mkdir") {
          if (arg.empty()) {
            std::cout << "`mkdir` missing required DIR. See `help`."
                      << std::endl;
          } else {
            std::cout << fs_->create_directory(arg) << std::endl;
          }
        } else if (command == "mkdirs") {
          if (arg.empty()) {
            std::cout << "`mkdirs` missing required DIR. See `help`."
                      << std::endl;
          } else {
            std::cout << fs_->create_directories(arg) << std::endl;
          }
        } else if (command == "abs") {
          if (arg.empty()) {
            std::cout << "`abs` missing required PATH. See `help`."
                      << std::endl;
          } else {
            std::cout << fs_->absolute(arg) << std::endl;
          }
        } else if (command == "stat") {
          if (arg.empty()) {
            std::cout << "`stat` missing required PATH. See `help`."
                      << std::endl;
          } else {
            auto status = fs_->status(arg);
            std::cout << "type: " << status.type() << '\n'
                      << "perms: " << status.permissions() << std::endl;
          }
        } else if (command == "exist") {
          if (arg.empty()) {
            std::cout << "`exist` missing required PATH. See `help`."
                      << std::endl;
          } else {
            std::cout << fs_->exists(arg) << std::endl;
          }
        } else if (command == "isdir") {
          if (arg.empty()) {
            std::cout << "`isdir` missing required DIR. See `help`."
                      << std::endl;
          } else {
            std::cout << fs_->is_directory(arg) << std::endl;
          }
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