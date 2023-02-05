#ifndef INCLUDED_PFS_STD_FILESYSTEM_HPP
#define INCLUDED_PFS_STD_FILESYSTEM_HPP

#include <pfs/filesystem.hpp>

namespace pfs {

class std_directory_entry final : public directory_entry {
private:
  const std::filesystem::directory_entry *ent_;

public:
  std_directory_entry &operator=(const std::filesystem::directory_entry &ent) {
    ent_ = &ent;
    return *this;
  }

  const pfs::path &path() const noexcept override { return ent_->path(); }

  file_status status() const override { return ent_->status(); }

  file_status status(error_code &ec) const override { return ent_->status(ec); }
};

class std_directory_iterator final : public directory_iterator {
private:
  inline static std::filesystem::directory_iterator end_{};
  std::filesystem::directory_iterator it_;
  mutable std_directory_entry ent_;

public:
  std_directory_iterator() = default;

  std_directory_iterator(const std::filesystem::directory_iterator &it)
      : it_(it) {}

  const directory_entry &operator*() const override {
    ent_ = *it_;
    return ent_;
  }

  directory_iterator &operator++() override {
    ++it_;
    return *this;
  }

  directory_iterator &incremenet(error_code &ec) override {
    it_.increment(ec);
    return *this;
  }

  std::uintptr_t hash() const override {
    if (it_ == end_) {
      return 0;
    } else {
      // TODO: Major assumption here. Is this right? I doubt it.
      // Returns address of the pointed-to std::filesystem::directory_entry.
      return reinterpret_cast<std::uintptr_t>(&(*it_));
    }
  }
};

class std_iterable_directory final : public iterable_directory {
private:
  std_directory_iterator begin_;
  std_directory_iterator end_;

public:
  std_iterable_directory(const std::filesystem::directory_iterator &it)
      : begin_(it) {}

  directory_iterator &begin() override { return begin_; }

  directory_iterator &end() override { return end_; }
};

class std_filesystem final : public filesystem {
public:
  path absolute(const path &p) override { return std::filesystem::absolute(p); }

  path absolute(const path &p, error_code &ec) override {
    return std::filesystem::absolute(p, ec);
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

  void rename(const path &old_p, const path &new_p) override {
    std::filesystem::rename(old_p, new_p);
  }

  void rename(const path &old_p, const path &new_p,
              error_code &ec) noexcept override {
    std::filesystem::rename(old_p, new_p, ec);
  }

  file_status status(const path &p) const override {
    return std::filesystem::status(p);
  }

  file_status status(const path &p, error_code &ec) const noexcept override {
    return std::filesystem::status(p, ec);
  }

  std::unique_ptr<iterable_directory>
  iterate_directory(const path &p) const override {
    std::filesystem::directory_iterator it(p);
    return std::make_unique<std_iterable_directory>(it);
  }

  std::unique_ptr<iterable_directory>
  iterate_directory(const path &p, error_code &ec) const override {
    std::filesystem::directory_iterator it(p, ec);
    return std::make_unique<std_iterable_directory>(it);
  }
};

} // namespace pfs

#endif