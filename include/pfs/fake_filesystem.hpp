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
   * @post The node list is sorted alphabetically by node name.
   *
   * @param l Node list to be modified.
   * @param n Node to insert.
   * @return true if the node was inserted; false if an equivalent node was
   * found and the input node was not inserted.
   */
  static bool insert_node(node_list &l, std::shared_ptr<node> n) {
    auto it = std::lower_bound(l.begin(), l.end(), n, [](auto n1, auto n2) {
      return n1->name < n2->name;
    });
    if (it == l.end() || (*it)->name != n->name) {
      // Not found in node list.
      l.insert(it, n);
      return true;
    } else {
      return false;
    }
  }

  /**
   * @brief Removes a node from a sorted node list.
   *
   * @pre The node list is sorted alphabetically by node name.
   *
   * @param l The node list to be modified.
   * @param n Removes all nodes with the same name.
   * @return true if any nodes were removed; false if none found.
   */
  static bool remove_node(node_list &l, std::shared_ptr<node> n) {
    auto [first, last] =
        std::equal_range(l.begin(), l.end(), n,
                         [](auto n1, auto n2) { return n1->name < n2->name; });
    if (first == last) {
      return false;
    }
    l.erase(first, last);
    return true;
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
   * @brief Executes a callback against all nodes in depth first-order
   *
   * @tparam Callback A Callable type of the form void(node&).
   * @param n Visit this node and its decendants.
   * @param visitor instance of the callback.
   */
  template <typename Callback>
  static void visit_nodes(node &n, Callback visitor) {
    visitor(n);
    for (auto dent : n.dents) {
      visit_nodes(*dent, visitor);
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
   * @return Iterator into the path indicating the deepest component that does
   * not exist in the node tree. If the path exists, this will be @c pend.
   */
  static path::const_iterator traverse(node_list &node_path,
                                       path::const_iterator pit,
                                       path::const_iterator pend) {
    if (pit == pend) {
      // Empty path. Traversal ends here.
      return pit;
    }
    if (*pit == ".") {
      // Traverse the current node for the next part of the path.
      return traverse(node_path, ++pit, pend);
    } else if (*pit == "..") {
      if (node_path.back()->name.root_directory().empty()) {
        // Last element of the path is NOT the root directory. Safe to pop.
        // Otherwise, remain in the root directory.
        node_path.pop_back();
      }
      return traverse(node_path, ++pit, pend);
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

  /**
   * @brief Traverses the node tree along the given path.
   *
   * @details If the input path @c p is relative, then traversal begins from the
   * current working directory. If it's absolute, then traversal begins from the
   * "meta"-root. If the input path does not exist in the filesystem, this will
   * traverse through the path until the next part does not exist.
   *
   * @param p Path to traverse.
   * @return A pair of values: (1) A list - starting with the meta-node - of
   * every existing node in the path. (2) An iterator into @c p of the next part
   * of the path that did not exist in the node tree.
   */
  std::pair<node_list, path::const_iterator> traverse(const path &p) const {
    node_list node_path;
    if (p.is_absolute()) {
      node_path.push_back(meta_root_);
    } else {
      node_path = cwd_nodes_;
    }
    auto pit = traverse(node_path, p.begin(), p.end());
    return {std::move(node_path), pit};
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

    // This node represents the root directory of the drive.
    auto root_dir_node = std::make_shared<node>();
    root_dir_node->name = "\\";
    root_dir_node->type = file_type::directory;
    root_node->dents.push_back(root_dir_node);

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
    if (p.empty()) {
      // Special case. Path is empty string.
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return false;
    }

    // Locate the node for the requested path.
    auto [node_path, pit] = traverse(p);

    if (pit == p.end()) {
      // The path already exists.
      if (node_path.back()->type == file_type::directory) {
        ec.clear();
      } else {
        ec = std::make_error_code(std::errc::not_a_directory);
      }
      return false;
    }

    if (++pit == p.end()) {
      // The parent path already exists.
      if (node_path.back()->type == file_type::directory) {
        auto new_dir = std::make_shared<node>();
        new_dir->type = file_type::directory;
        new_dir->name = p.stem();
        insert_node(node_path.back()->dents, new_dir);
        ec.clear();
        return true;
      }
    }

    // Either the parent does not exist, or it is not a directory.
    ec = std::make_error_code(std::errc::no_such_file_or_directory);
    return false;
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

    auto [node_path, pit] = traverse(p);
    if (pit == p.end()) {
      // Path already exists.
      if (node_path.back()->type == file_type::directory) {
        ec.clear();
      } else {
        ec = std::make_error_code(std::errc::not_a_directory);
      }
      return false;
    }

    if (node_path.back()->type != file_type::directory) {
      // Deepest existing node is not a directory. Cannot make additional
      // directories from here.
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return false;
    }

    // Make additional directories.
    auto parent_node = node_path.back();
    for (; pit != p.end(); ++pit) {
      auto new_dir = std::make_shared<node>();
      new_dir->name = *pit;
      new_dir->type = file_type::directory;
      insert_node(parent_node->dents, new_dir);
      parent_node = new_dir;
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

  path current_path(error_code &ec) const noexcept {
    ec.clear();
    return cwd_;
  }

  path current_path() const { return cwd_; }

  void current_path(const path &p, error_code &ec) noexcept {
    if (p.empty()) {
      ec = std::make_error_code(std::errc::no_such_file_or_directory);
      return;
    }

    auto [node_path, pit] = traverse(p);
    if (pit == p.end()) {
      // Path exists
      if (node_path.back()->type == file_type::directory) {
        cwd_ = p;
        cwd_nodes_ = std::move(node_path);
      } else {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
      }
      return;
    }
  }

  void current_path(const path &p) {
    error_code ec;
    current_path(p, ec);
    if (ec) {
      throw filesystem_error("current_path", ec);
    }
  }

  bool exists(const path &p, error_code &ec) const noexcept override {
    ec.clear();
    if (p.empty()) {
      // Special case. Path is empty string.
      return false;
    }
    auto [node_path, pit] = traverse(p);
    return pit == p.end();
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
    auto [node_path, pit] = traverse(p);
    return (pit == p.end() && node_path.back()->type == file_type::directory);
  }

  bool is_directory(const path &p) const override {
    error_code ec;
    bool ret = is_directory(p, ec);
    if (ec) {
      throw filesystem_error("is_directory", ec);
    }
    return ret;
  }

  bool remove(const path &p, error_code &ec) noexcept {
    if (p.empty()) {
      ec.clear();
      return false;
    }
    auto [node_path, pit] = traverse(p);
    if (pit != p.end()) {
      // Path does not exist
      ec.clear();
      return false;
    }
    auto n = node_path.back();
    if (n->type == file_type::directory) {
      if (!n->name.root_directory().empty()) {
        // Cannot remove the root directory.
        ec = std::make_error_code(std::errc::permission_denied);
        return false;
      } else if (!n->dents.empty()) {
        // Cannot remove non-empty directories.
        ec = std::make_error_code(std::errc::directory_not_empty);
        return false;
      } else {
        // Remove the empty directory.
        auto dir = node_path.back();
        node_path.pop_back();
        auto parent = node_path.back();
        remove_node(parent->dents, dir);
        ec.clear();
        return true;
      }
    }

    // TODO: Handle types other than directories.
    ec = std::make_error_code(std::errc::not_supported);
    return false;
  }

  bool remove(const path &p) {
    error_code ec;
    auto ret = remove(p, ec);
    if (ec) {
      throw filesystem_error("remove", ec);
    }
    return ret;
  }

  std::uintmax_t remove_all(const path &p, error_code &ec) noexcept override {
    if (p.empty()) {
      ec.clear();
      return 0;
    }
    auto [node_path, pit] = traverse(p);
    if (pit != p.end()) {
      // Path does not exist.
      ec.clear();
      return 0;
    }
    auto n = node_path.back();
    if (n->type == file_type::directory) {
      if (!n->name.root_directory().empty()) {
        // Cannot remove the root directory.
        ec = std::make_error_code(std::errc::permission_denied);
        return false;
      }
    }

    // Unlink the node from its parent.
    node_path.pop_back();
    auto parent = node_path.back();
    remove_node(parent->dents, n);

    // Count decendants
    std::uintmax_t count = 0;
    visit_nodes(*n, [&count](auto &) { ++count; });

    // Release the unlinked node. Free the memory.
    n.reset();
    return count;
  }

  std::uintmax_t remove_all(const path &p) override {
    error_code ec;
    auto ret = remove_all(p, ec);
    if (ec) {
      throw filesystem_error("remove_all", ec);
    }
    return ret;
  }

  file_status status(const path &p) const override {
    file_status s;
    auto [node_path, pit] = traverse(p);
    if (pit == p.end()) {
      s.type(node_path.back()->type);
    } else {
      s.type(file_type::not_found);
    }
    return s;
  }
};

} // namespace pfs

#endif