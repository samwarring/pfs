#ifndef INCLUDED_PFS_FAKE_FILESYSTEM_HPP
#define INCLUDED_PFS_FAKE_FILESYSTEM_HPP

#include <map>
#include <memory>
#include <pfs/filesystem.hpp>

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
   *
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
   *
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

    // Skip past the root directory.
    ++it;

    // Iterate through remaining path.
    for (; it != p.end(); ++it) {
      auto dent = n->dents.find(it->string());
      if (dent == n->dents.end()) {
        return nullptr;
      }
      n = dent->second.get();
    }

    return n;
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
   *
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
    }
    return s;
  }
};

} // namespace pfs

#endif