#ifndef INCLUDED_PFS_FILESYSTEM_HPP
#define INCLUDED_PFS_FILESYSTEM_HPP

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>

namespace pfs {

using std::error_code;
using std::filesystem::file_status;
using std::filesystem::file_type;
using std::filesystem::filesystem_error;
using std::filesystem::path;

class filesystem;
class iterable_directory;
class directory_iterator;
class directory_entry;

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
  virtual std::unique_ptr<iterable_directory>
  iterate_directory(const path &p) const = 0;
  virtual std::unique_ptr<iterable_directory>
  iterate_directory(const path &p, error_code &ec) const = 0;
};

class iterable_directory {
public:
  virtual ~iterable_directory() = default;
  virtual std::unique_ptr<directory_iterator> begin() const = 0;
  virtual std::unique_ptr<directory_iterator> end() const = 0;
};

class directory_iterator {
public:
  using value_type = directory_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = const directory_entry *;
  using reference = const directory_entry &;
  using iterator_category = std::input_iterator_tag;

  virtual const directory_entry &operator*() const = 0;

  const directory_entry *operator->() const { return &(**this); }

  virtual directory_iterator &operator++() = 0;
  virtual directory_iterator &incremenet(error_code &ec) = 0;
  virtual std::uintptr_t hash() const = 0;

  bool operator==(const directory_iterator &rhs) const {
    return hash() == rhs.hash();
  }

  bool operator!=(const directory_iterator &rhs) const {
    return hash() != rhs.hash();
  }
};

class directory_entry {
public:
  virtual void assign(const path &p) = 0;
  virtual void assign(const path &p, error_code &ec) = 0;
  virtual void replace_filename(const path &p) = 0;
  virtual void replace_filename(const path &p, error_code &ec) = 0;
  virtual void refresh() = 0;
  virtual void refresh(error_code &ec) = 0;
  virtual const path &path() const noexcept = 0;

  operator const pfs::path &() const noexcept { return path(); }

  virtual file_status status() const = 0;
  virtual file_status status(error_code &ec) = 0;
};

} // namespace pfs

#endif