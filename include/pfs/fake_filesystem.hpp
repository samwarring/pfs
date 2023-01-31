#ifndef INCLUDED_PFS_FAKE_FILESYSTEM_HPP
#define INCLUDED_PFS_FAKE_FILESYSTEM_HPP

#include <map>
#include <memory>
#include <pfs/filesystem.hpp>
#include <utility>

namespace pfs {

class fake_filesystem final : public filesystem {
private:
  struct node {
    file_type type;
    std::map<std::string, std::shared_ptr<node>> dents;
  };

  std::map<std::string, node> roots_;

  /**
   * @brief Gets pointer to the root node for this path.
   *
   * @pre The argument is an absolute path.
   *
   * @param p path object.
   * @return pointer to the root node, or nullptr if not found.
   */
  const node *find_root(const path &p) const {
    auto it = roots_.find(p.root_name().string());
    if (it == roots_.end()) {
      return nullptr;
    }
    return &(it->second);
  }

  /**
   * @brief Gets pointer to the node described by this path
   *
   * @pre The argument is an absolute path.
   *
   * @param p path object.
   * @return pointer to the filesystem node, or nullptr if not found.
   */
  const node *find_node(const path &p) const {
    const node *n = find_root(p);
    if (!n) {
      return nullptr;
    }
    auto it = p.begin();
    if (!p.root_name().empty()) {
      // Skip past the root name.
      ++it;
    }
    return find_node(++it, p.end(), *n);
  }

  /**
   * @brief Finds a node in the filesystem from a path.
   *
   * @details This can be used to lookup relative paths if @c n is the current
   * working directory, and @c it is the beginning of the relative path. If the
   * initial path is absolute, then the initial iterator @c it should start from
   * the root directory, skipping past the root_name if present.
   *
   * @param it Iterator through the path compoments.
   * @param end End of the path, as an iterator.
   * @param n Begin search from this node in the filesystem.
   * @return Pointer to the requested node if found, or nullptr if not found.
   */
  const node *find_node(path::const_iterator it, path::const_iterator end,
                        const node &n) const {
    if (it == end) {
      return &n;
    }
    auto dent = n.dents.find(it->string());
    if (dent == n.dents.end()) {
      // Next part of path not found under this node.
      return nullptr;
    }
    return find_node(++it, end, *dent->second);
  }

  /**
   * @brief Similar to @ref find_node, but returns the longest existing path if
   * the requested node does not exist.
   *
   * @pre The argment is an absolute path.
   *
   * @param p Finds node corresponding to this path.
   * @return A Pair of values. (First) If the path exists, node for the path; if
   * the path does not exist, the deepest existing node along the path; if the
   * root does not exist, nullptr. (Second) Iterator into the path corresponding
   * to the returned node; if the root was not found, the iterator is undefined
   * and should be ignored.
   */
  std::pair<const node *, path::const_iterator>
  find_deepest_existing_node(const path &p) const {
    const node *n = find_root(p);
    if (!n) {
      return {nullptr, p.begin()};
    }
    auto it = p.begin();
    if (!p.root_name().empty()) {
      // Skip past the root name.
      ++it;
    }
    return find_deepest_existing_node(++it, p.end(), *n);
  }

  /**
   * @brief Similar to @ref find_node, but returns the longest existing path if
   * the requested node does not exist.
   *
   * @param it Iterator through the path components.
   * @param end End of the path components.
   * @param n Begin search from this node in the filesystem.
   * @return A pair of values: (First) Pointer to the requested node if it
   * exists, or pointer to the deepest-existing node along the path if the
   * requested node does not exist. (Second), iterator into the path
   * corresponding to the returned node.
   */
  std::pair<const node *, path::const_iterator>
  find_deepest_existing_node(path::const_iterator it, path::const_iterator end,
                             const node &n) const {
    if (it == end) {
      return {&n, it};
    }
    auto dent = n.dents.find(it->string());
    if (dent == n.dents.end()) {
      // Next part of path not found. Return the current node.
      return {&n, it};
    }
    return find_deepest_existing_node(++it, end, *dent->second);
  }

public:
  /**
   * @brief Adds a new root to the filesystem.
   *
   * @details std::filesystem defines the concept of "root name" for all path
   * objects, separate from the root directory. On Windows, the root name can
   * be "C:" or "//myserver". On linux, the root name is an empty string. The
   * fake filesystem must contain a root before any paths can be added to it.
   *
   * @param root_name To simulate a Windows filesystem, this can be "C:". To
   * simulate a POSIX filesystem, this can be an empty string.
   * @return true if the root was created, or false if it already existed.
   */
  bool create_root(std::string root_name) {
    node root_dir;
    root_dir.type = file_type::directory;
    auto [iter, inserted] = roots_.try_emplace(std::move(root_name), root_dir);
    return inserted;
  }

public:
  file_status status(const path &p) const override {
    file_status s;
    const node *n = find_node(p);
    if (n) {
      s.type(n->type);
    } else {
      s.type(file_type::not_found);
    }
    return s;
  }

  bool create_directory(const path &p, error_code &ec) noexcept override {
    // todo: assume absolute path for now.
    if (p.empty()) {
      // Special case. Path is empty string.
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return false;
    }
    const node *n = find_node(p.parent_path());
    if (!n) {
      // Parent path does not exist. Not an error, but no directory created.
      ec.clear();
      return false;
    }
    auto target = n->dents.find(p.stem().string());
    if (target != n->dents.end()) {
      if (target->second->type == file_type::directory) {
        // Directory already exists.
        ec.clear();
        return false;
      } else {
        // Target path exists, but it's not a directory.
        ec = std::make_error_code(std::errc::not_a_directory);
        return false;
      }
    }
    // Target path does not exist. Make the directory.
    auto new_dir = std::make_shared<node>();
    new_dir->type = file_type::directory;
    const_cast<node *>(n)->dents[p.stem().string()] = new_dir;
    ec.clear();
    return true;
  }

  bool create_directory(const path &p) override {
    error_code ec;
    bool ret = create_directory(p, ec);
    if (ec) {
      throw filesystem_error("create_directory", ec);
    }
    return ret;
  }

  bool create_directories(const path &p, error_code &ec) noexcept override {
    if (p.empty()) {
      // Special case. Path is empty string.
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return false;
    }
    auto [n, it] = find_deepest_existing_node(p);
    if (!n && p.relative_path().empty()) {
      // Special case. If path is non-existent root without any relative path,
      // it is not an error and no directories are created. (This is the case on
      // Windows. Need to confirm on POSIX).
      ec.clear();
      return false;
    } else if (!n) {
      // Root does not exist. This is an error.
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return false;
    } else if (n->type != file_type::directory) {
      // Deepest existing node is not a directory.
      ec = std::make_error_code(std::errc::not_a_directory);
      return false;
    } else if (it == p.end()) {
      // Requested path is already a directory.
      ec.clear();
      return false;
    }
    for (; it != p.end(); ++it) {
      auto next_dir = std::make_shared<node>();
      next_dir->type = file_type::directory;
      const_cast<node *>(n)->dents[it->string()] = next_dir;
      n = next_dir.get();
    }
    ec.clear();
    return true;
  }

  bool create_directories(const path &p) override {
    error_code ec;
    bool ret = create_directories(p, ec);
    if (ec) {
      throw filesystem_error("create_directories", ec);
    }
    return ret;
  }

  bool exists(const path &p, error_code &ec) const noexcept override {
    ec.clear();
    if (p.empty()) {
      // Special case. Path is empty string.
      return false;
    }
    return find_node(p) != nullptr;
  }

  bool exists(const path &p) const override {
    error_code ec;
    bool ret = exists(p, ec);
    if (ec) {
      throw filesystem_error("exists", ec);
    }
    return ret;
  }

  bool is_directory(const path &p, error_code &ec) const noexcept override {
    ec.clear();
    if (p.empty()) {
      // Special case. Path is empty string.
      return false;
    }
    auto n = find_node(p);
    return n && n->type == file_type::directory;
  }

  bool is_directory(const path &p) const override {
    error_code ec;
    bool ret = is_directory(p, ec);
    if (ec) {
      throw filesystem_error("is_directory", ec);
    }
    return ret;
  }
};

} // namespace pfs

#endif