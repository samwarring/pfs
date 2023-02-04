#include <iostream>
#include <pfs/fake_filesystem.hpp>
#include <pfs/std_filesystem.hpp>
#include <sstream>
#include <string>

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
            std::cout << "`abs` missing required PATH. See `help`.";
          } else {
            std::cout << fs_->absolute(arg) << std::endl;
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