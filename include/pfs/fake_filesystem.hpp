#ifndef INCLUDED_PFS_FAKE_FILESYSTEM_HPP
#define INCLUDED_PFS_FAKE_FILESYSTEM_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <pfs/filesystem.hpp>
#include <utility>

namespace pfs {

class fake_filesystem final : public filesystem {
private:
  struct node;
  using node_list = std::vector<std::shared_ptr<node>>;

  struct node {
    path name;
    file_type type;
    bool is_drive;
    node_list dents;
  };

  std::shared_ptr<node> meta_root_;
  node_list cwd_nodes_;
  path cwd_;

  /**
   * @brief Adds a node to the sorted node list.
   *
   * @pre The node list is sorted alphabetically by node name.
   *
   * @param l Node list to be modified.
   * @param n Node to insert.
   * @return true if the node was inserted; false if an equivalent node was
   * found and the input node was not inserted.
   */
  static bool insert_node(node_list &l, std::shared_ptr<node> n) {
    auto it = std::upper_bound(l.begin(), l.end(), n, [](auto n1, auto n2) {
      return n1->name < n2->name;
    });
    if ((*it)->name == n->name) {
      return false;
    } else {
      l.insert(it, n);
      return true;
    }
  }

  /**
   * @brief Finds a node in a sorted node list.
   *
   * @pre The node list is sorted alphabetically by node name.
   *
   * @param l Node list to be searched.
   * @param name Name of the node to search for.
   * @return The found node, or nullptr if not found.
   */
  static std::shared_ptr<node> find_node(const node_list &l, const path &name) {
    auto val = std::make_shared<node>();
    val->name = name;
    auto [first, last] =
        std::equal_range(l.begin(), l.end(), val,
                         [](auto n1, auto n2) { return n1->name < n2->name; });
    if (first == last) {
      // Not found.
      return nullptr;
    } else {
      return *first;
    }
  }

  /**
   * @brief Traverses the node tree along a path.
   *
   * @param node_path The caller initializes this with the node where traversal
   * begins. When this function returns, it contains the path of existing nodes
   * that were traversed.
   * @param pit Iterator to a path that directs the traversal.
   * @param pend Stop traversal when @c pit equals this value.
   * @return Iterator into the path indicating the deepest component that exists
   * in the node tree.
   */
  static path::const_iterator traverse(node_list &node_path,
                                       path::const_iterator pit,
                                       path::const_iterator pend) {
    if (pit == pend) {
      // Empty path. Traversal ends here.
      return pit;
    }
    auto next = find_node(node_path.back()->dents, *pit);
    if (!next) {
      // Next part of the path not found. Traversal ends here.
      return pit;
    } else {
      // Traverse the remainder of the path from the found node.
      node_path.push_back(next);
      return traverse(node_path, ++pit, pend);
    }
  }

public:
  /**
   * @brief Adds a new root to the filesystem.
   *
   * @details If the current platform is POSIX, this function is meaningless
   * because it only allows a root name of "" (empty string). If the current
   * platform is Windows, this function allows adding additional drive letters,
   * each with their own root directory.
   *
   * @param root_name If compiling on Windows, must consist of a drive-letter
   * followed by colon (e.g. "D:"). If compiling on POSIX, must be an empty
   * string. The parameter must not contain a root directory or relative path.
   * @return true if the root was created, or false if it already existed.
   * @throw std::invalid_argument if @c root_name is not a valid root for the
   * current platform.
   */
  bool create_root(const path &root_name) {
    bool is_root_only = false;
#ifdef _WIN32
    is_root_only = !root_name.root_name().empty() &&
                   root_name.root_directory().empty() &&
                   root_name.relative_path().empty();
#else
    is_root_only = root_name.empty();
#endif
    if (!is_root_only) {
      throw std::invalid_argument(
          "\"" + root_name.string() +
          "\" is not a valid root name for this platform");
    }

    auto root_node = std::make_shared<node>();
#ifdef _WIN32
    // This node represents the drive.
    root_node->name = root_name;
    root_node->type = file_type::none;
    auto root_dir_node = std::make_shared<node>();

    // This node represents the root directory of the drive.
    root_dir_node->name = "\\";
    root_dir_node->type = file_type::directory;
    root_node->dents.push_back(std::make_shared<node>());

    // If cwd not set, set it now.
    if (cwd_.empty()) {
      cwd_ = root_name / "\\";
      cwd_nodes_.push_back(root_node);
      cwd_nodes_.push_back(root_dir_node);
    }
#else
    // This node represents the root directory.
    root_node->name = "/";

    // If cwd not set, set it now.
    if (cwd_.empty()) {
      cwd_ = "/";
      cwd_nodes_.push_back(root_node);
    }
#endif

    auto existing_root = find_node(meta_root_->dents, root_node->name);
    if (existing_root) {
      // Requested root already exists.
      return false;
    } else {
      insert_node(meta_root_->dents, root_node);
      return true;
    }
  }

  /**
   * @brief Gets path to the default root directory for the current platform.
   * @return path containing optional root name (on Windows, "C:"), and root
   * directory.
   */
  path default_root() const {
#ifdef _WIN32
    return "C:\\";
#else
    return "/";
#endif
  }

  /**
   * @brief Constructs a new fake filesystem.
   *
   * @details The new filesystem consists only of a optional root name (in the
   * case of Windows, "C:") with a root directory. The root directory is
   * initially empty.
   */
  fake_filesystem() {
    meta_root_ = std::make_shared<node>();
#ifdef _WIN32
    path root_name = "C:";
#else
    path root_name;
#endif
    create_root(root_name);
  }

public:
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
};

} // namespace pfs

#endif