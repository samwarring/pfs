#ifndef INCLUDED_PFS_FILESYSTEM_HPP
#define INCLUDED_PFS_FILESYSTEM_HPP

#include <filesystem>

namespace pfs {

using std::error_code;
using std::filesystem::file_status;
using std::filesystem::file_type;
using std::filesystem::filesystem_error;
using std::filesystem::path;

class filesystem {
public:
  virtual file_status status(const path &p) const = 0;
  virtual bool create_directory(const path &p) = 0;
  virtual bool create_directory(const path &p, error_code &ec) noexcept = 0;
  virtual bool create_directories(const path &p) = 0;
  virtual bool create_directories(const path &p, error_code &ec) noexcept = 0;
  virtual bool exists(const path &p) const = 0;
  virtual bool exists(const path &p, error_code &ec) const noexcept = 0;
  virtual bool is_directory(const path &p) const = 0;
  virtual bool is_directory(const path &p, error_code &ec) const noexcept = 0;
};

} // namespace pfs

#endif