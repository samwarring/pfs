#include <iostream>
#include <pfs/fake_filesystem.hpp>
#include <pfs/std_filesystem.hpp>
#include <sstream>
#include <stdexcept>
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

class invalid_command_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class application {
private:
  pfs::std_filesystem real_fs_;
  pfs::fake_filesystem fake_fs_;
  pfs::filesystem *fs_{&fake_fs_};

  static void print_help() {
    std::cout
        << '\n'
        << "Available commands:\n"
        << '\n'
        << "  h, help        Print this message.\n"
        << "  real           Switch to real filesystem.\n"
        << "  fake           Switch to fake filesystem.\n"
        << "  pwd            Print working directory.\n"
        << "  cd DIR         Change working directory.\n"
        << "  ls [DIR]       List contents of directory.\n"
        << "  lr [DIR]       Recursively list contents of directory.\n"
        << "  li [DIR]       Interactively recurse directory contents.\n"
        << "  mkdir DIR      Create new directory. Parent must exist.\n"
        << "  mkdirs DIR     Create directory and subdirectories.\n"
        << "  rm PATH        Remove file or empty directory.\n"
        << "  rmr PATH       Remove file or directories recursively.\n"
        << "  mv SRC DST     Rename or move file or directory.\n"
        << "  abs PATH       Convert to absolute path.\n"
        << "  stat PATH      Prints properties file or directory.\n"
        << "  exist PATH     Checks if the path exists.\n"
        << "  isdir PATH     Checks if the path is a directory.\n"
        << "  path PATH      Decompose a path.\n"
        << "  touch FILE     Update file timestamp. Create if necessary.\n"
        << "  cat FILE       Print the contents of a file.\n"
        << "  x, exit        Exit this program.\n"
        << std::endl;
  }

  static void print_help_interactive_recursive_list() {
    std::cout << '\n'
              << "You have entered interactive recursive list mode. This\n"
              << "mode uses different commands. To return to normal mode\n"
              << "use the `x` or `exit` commands.\n"
              << '\n'
              << "Available Commands:\n"
              << '\n'
              << "  h, help  Print this help message.\n"
              << "  i        Step into. If not directory, step over.\n"
              << "  n        Step over. If directory, do not enter.\n"
              << "  o        Step out. Leave current directory.\n"
              << "  d        Print current depth.\n"
              << "  x, exit  Return to normal mode.\n"
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

  void print_lsri_prompt(pfs::recursive_directory_iterator &it) {
    std::cout << (fs_ == &real_fs_ ? "[real] " : "[fake] ")
              << it.status().permissions() << "  " << std::setw(9) << std::left
              << it.status().type() << "  [" << it.path().string() << "] ?> ";
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
      std::ostringstream sout;
      sout << "`" << command_name << "` missing required " << metavar
           << ". See `help`.";
      throw invalid_command_error(sout.str());
    }
    return false;
  }

  static bool parsed(const std::vector<std::string> &tokens,
                     const char *command_name, const char *metavar1,
                     const char *metavar2) {
    if (tokens[0] == command_name) {
      std::ostringstream sout;
      if (tokens.size() == 1) {
        sout << "`" << command_name << "` missing required " << metavar1
             << " and " << metavar2 << ". See `help`.";
        throw invalid_command_error(sout.str());
      }
      if (tokens.size() == 2) {
        sout << "`" << command_name << "` missing required " << metavar2
             << ". See `help`.";
        throw invalid_command_error(sout.str());
      }
      return true;
    }
    return false;
  }

  void interactive_recursive_list(pfs::path p) {
    auto it = fs_->recursive_directory_iterator(p);
    if (it->at_end()) {
      std::cout << "The directory is empty." << std::endl;
      return;
    }
    print_help_interactive_recursive_list();
    try {
      for (;;) {
        print_lsri_prompt(*it);
        auto tokens = read_command();
        if (tokens.empty()) {
          continue;

        } else if (parsed(tokens, "h") || parsed(tokens, "help")) {
          print_help_interactive_recursive_list();

        } else if (parsed(tokens, "x") || parsed(tokens, "exit")) {
          std::cout << "Returning to normal mode." << std::endl;
          break;

        } else if (parsed(tokens, "i")) {
          it->increment();

        } else if (parsed(tokens, "n")) {
          it->disable_recursion_pending();
          it->increment();

        } else if (parsed(tokens, "o")) {
          it->pop();

        } else if (parsed(tokens, "d")) {
          std::cout << "Depth: " << it->depth() << std::endl;

        } else {
          std::cout << "Unrecognized command. Try running `help`." << std::endl;
        }

        if (it->at_end()) {
          std::cout << "Recursive iteration complete. Returning to "
                       "normal mode."
                    << std::endl;
          break;
        }
      }

    } catch (const std::filesystem::filesystem_error &e) {
      std::cout << "Caught exception: " << e.what() << '\n'
                << "Returning to normal mode." << std::endl;
    };
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

        } else if (parsed(tokens, "h") || parsed(tokens, "help")) {
          print_help();

        } else if (parsed(tokens, "x") || parsed(tokens, "exit")) {
          break;

        } else if (parsed(tokens, "real")) {
          fs_ = &real_fs_;

        } else if (parsed(tokens, "fake")) {
          fs_ = &fake_fs_;

        } else if (parsed(tokens, "pwd")) {
          std::cout << fs_->current_path().string() << std::endl;

        } else if (parsed(tokens, "cd", "DIR")) {
          fs_->current_path(tokens[1]);

        } else if (parsed(tokens, "ls")) {
          std::string target = tokens.size() > 1 ? tokens[1] : ".";
          for (auto it = fs_->directory_iterator(target); !it->at_end();
               it->increment()) {
            std::cout << it->status().permissions() << "  " << std::setw(9)
                      << std::left << it->status().type() << "  "
                      << it->path().filename().string() << std::endl;
          }

        } else if (parsed(tokens, "lr")) {
          std::string target = tokens.size() > 1 ? tokens[1] : ".";
          for (auto it = fs_->recursive_directory_iterator(target);
               !it->at_end(); it->increment()) {
            std::cout << it->status().permissions() << "  " << std::setw(9)
                      << std::left << it->status().type() << "  "
                      << it->path().string() << std::endl;
          }

        } else if (parsed(tokens, "li")) {
          std::string target = tokens.size() > 1 ? tokens[1] : ".";
          interactive_recursive_list(target);

        } else if (parsed(tokens, "mkdir", "DIR")) {
          std::cout << fs_->create_directory(tokens[1]) << std::endl;

        } else if (parsed(tokens, "mkdirs", "DIR")) {
          std::cout << fs_->create_directories(tokens[1]) << std::endl;

        } else if (parsed(tokens, "rm", "PATH")) {
          std::cout << fs_->remove(tokens[1]) << std::endl;

        } else if (parsed(tokens, "rmr", "PATH")) {
          std::cout << fs_->remove_all(tokens[1]) << std::endl;

        } else if (parsed(tokens, "mv", "SRC", "DST")) {
          fs_->rename(tokens[1], tokens[2]);

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

        } else if (parsed(tokens, "path", "PATH")) {
          pfs::path p(tokens[1]);
          std::cout << "Path: \"" << p.string() << "\"\n"
                    << "Root Name: \"" << p.root_name().string() << "\"\n"
                    << "Root Directory: \"" << p.root_directory().string()
                    << "\"\n"
                    << "Relative Path: \"" << p.relative_path().string()
                    << "\"\n"
                    << "Filename: \"" << p.filename().string() << "\"\n"
                    << "Stem: \"" << p.stem().string() << "\"\n"
                    << "Extension: \"" << p.extension().string() << "\""
                    << std::endl;
          std::cout << "Iteration: ";
          for (auto part : p) {
            std::cout << "\"" << part.string() << "\" ";
          }
          std::cout << std::endl;

        } else if (parsed(tokens, "touch", "FILE")) {
          fs_->open_file_w(tokens[1], std::ios_base::app);

        } else if (parsed(tokens, "cat", "FILE")) {
          auto in = fs_->open_file_r(tokens[1]);
          if (in->fail()) {
            std::cout << "The file could not be opened." << std::endl;
          } else {
            std::cout << in->rdbuf();
            if (std::cout.fail()) {
              // This can happen if the input file is empty.
              std::cout.clear();
            }
            if (in->fail()) {
              // Unable to dump the entire file.
              std::cout << "Error reading the file." << std::endl;
            }
          }

        } else {
          std::cout << "Unrecognized command. Try running `help`." << std::endl;
        }
      } catch (const invalid_command_error &e) {
        std::cout << "Invalid command. " << e.what() << std::endl;
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