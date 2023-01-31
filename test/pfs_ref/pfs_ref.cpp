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

class subcommand {
protected:
  CLI::App *parser;

public:
  subcommand(CLI::App &parent_parser, std::string name) {
    parser = parent_parser.add_subcommand(name);
  }

  virtual ~subcommand() {}

  virtual void run() = 0;

  void run_if_parsed() {
    if (parser->parsed()) {
      run();
    }
  }
};

struct subcommand_current_path : public subcommand {
  std::string set_path;

  subcommand_current_path(CLI::App &app) : subcommand(app, "current_path") {
    parser->add_option("--set", set_path)->option_text("PATH");
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

int main(int argc, char **argv) {

  CLI::App app{"Peforms arbitrary std::filesystem operations, so their "
               "behavior can be replicated in pfs."};

  std::unique_ptr<subcommand> subcommands[]{
      std::make_unique<subcommand_current_path>(app)};

  CLI11_PARSE(app, argc, argv);

  for (auto &s : subcommands) {
    s->run_if_parsed();
  }
}