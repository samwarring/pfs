#ifndef INCLUDED_PFS_STD_FILESYSTEM_HPP
#define INCLUDED_PFS_STD_FILESYSTEM_HPP

#include <pfs/filesystem.hpp>

namespace pfs {

class std_filesystem final : public filesystem {
public:
  bool create_directory(const path &p) override {
    return std::filesystem::create_directory(p);
  }

  bool create_directory(const path &p, error_code &ec) noexcept override {
    return std::filesystem::create_directory(p, ec);
  }

  bool create_directories(const path &p) override {
    return std::filesystem::create_directories(p);
  }

  bool create_directories(const path &p, error_code &ec) noexcept override {
    return std::filesystem::create_directories(p, ec);
  }

  path current_path() const { return std::filesystem::current_path(); }

  path current_path(error_code &ec) const noexcept {
    return std::filesystem::current_path(ec);
  }

  void current_path(const path &p) { std::filesystem::current_path(p); }

  void current_path(const path &p, error_code &ec) noexcept {
    std::filesystem::current_path(p, ec);
  }

  bool exists(const path &p) const override {
    return std::filesystem::exists(p);
  }

  bool exists(const path &p, error_code &ec) const noexcept override {
    return std::filesystem::exists(p, ec);
  }

  bool is_directory(const path &p) const override {
    return std::filesystem::is_directory(p);
  }

  bool is_directory(const path &p, error_code &ec) const noexcept override {
    return std::filesystem::is_directory(p);
  }

  bool remove(const path &p) { return std::filesystem::remove(p); }

  bool remove(const path &p, error_code &ec) noexcept {
    return std::filesystem::remove(p, ec);
  }

  std::uintmax_t remove_all(const path &p) override {
    return std::filesystem::remove_all(p);
  }

  std::uintmax_t remove_all(const path &p, error_code &ec) noexcept override {
    return std::filesystem::remove_all(p, ec);
  }

  file_status status(const path &p) const override {
    return std::filesystem::status(p);
  }
};

} // namespace pfs

#endif