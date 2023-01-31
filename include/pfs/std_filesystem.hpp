#ifndef INCLUDED_PFS_STD_FILESYSTEM_HPP
#define INCLUDED_PFS_STD_FILESYSTEM_HPP

#include <pfs/filesystem.hpp>

namespace pfs {

class std_filesystem final : public filesystem {
public:
  file_status status(const path &p) const override {
    return std::filesystem::status(p);
  }

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
};

} // namespace pfs

#endif