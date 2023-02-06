#ifndef INCLUDED_PFS_FILESYSTEM_HPP
#define INCLUDED_PFS_FILESYSTEM_HPP

#include <cstddef>
#include <filesystem>
#include <memory>

namespace pfs {

using std::error_code;
using std::filesystem::file_status;
using std::filesystem::file_type;
using std::filesystem::filesystem_error;
using std::filesystem::path;

class filesystem;
class directory_iterator;
class recursive_directory_iterator;

class filesystem {
public:
  virtual ~filesystem() = default;
  virtual path absolute(const path &p) = 0;
  virtual path absolute(const path &p, error_code &ec) = 0;
  virtual bool create_directory(const path &p) = 0;
  virtual bool create_directory(const path &p, error_code &ec) noexcept = 0;
  virtual bool create_directories(const path &p) = 0;
  virtual bool create_directories(const path &p, error_code &ec) noexcept = 0;
  virtual path current_path() const = 0;
  virtual path current_path(error_code &ec) const noexcept = 0;
  virtual void current_path(const path &p) = 0;
  virtual void current_path(const path &p, error_code &ec) noexcept = 0;
  virtual bool exists(const path &p) const = 0;
  virtual bool exists(const path &p, error_code &ec) const noexcept = 0;
  virtual bool is_directory(const path &p) const = 0;
  virtual bool is_directory(const path &p, error_code &ec) const noexcept = 0;
  virtual bool remove(const path &p) = 0;
  virtual bool remove(const path &p, error_code &ec) noexcept = 0;
  virtual std::uintmax_t remove_all(const path &p) = 0;
  virtual std::uintmax_t remove_all(const path &p, error_code &ec) noexcept = 0;
  virtual void rename(const path &old_p, const path &new_p) = 0;
  virtual void rename(const path &old_p, const path &new_p,
                      error_code &ec) noexcept = 0;
  virtual file_status status(const path &p) const = 0;
  virtual file_status status(const path &p, error_code &ec) const noexcept = 0;
  virtual std::unique_ptr<pfs::directory_iterator>
  directory_iterator(const path &p) const = 0;
  virtual std::unique_ptr<pfs::directory_iterator>
  directory_iterator(const path &p, error_code &ec) const = 0;
  virtual std::unique_ptr<pfs::recursive_directory_iterator>
  recursive_directory_iterator(const path &p) const = 0;
  virtual std::unique_ptr<pfs::recursive_directory_iterator>
  recursive_directory_iterator(const path &p, error_code &ec) const = 0;
};

class directory_iterator {
public:
  virtual directory_iterator &increment() = 0;
  virtual directory_iterator &increment(error_code &ec) = 0;
  virtual bool at_end() const = 0;
  virtual const path &path() const noexcept = 0;
  virtual file_status status() const = 0;
  virtual file_status status(error_code &ec) const = 0;
};

class recursive_directory_iterator {
public:
  virtual recursive_directory_iterator &increment() = 0;
  virtual recursive_directory_iterator &increment(error_code &ec) = 0;
  virtual bool at_end() const = 0;
  virtual bool recursion_pending() const = 0;
  virtual void pop() = 0;
  virtual void pop(error_code &ec) = 0;
  virtual void disable_recursion_pending() = 0;
  virtual const path &path() const noexcept = 0;
  virtual file_status status() const = 0;
  virtual file_status status(error_code &ec) const = 0;
};

} // namespace pfs

#endif