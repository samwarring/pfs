#ifndef INCLUDED_PFS_FILESYSTEM_HPP
#define INCLUDED_PFS_FILESYSTEM_HPP

#include <filesystem>

namespace pfs {

using std::error_code;
using std::filesystem::file_status;
using std::filesystem::file_type;
using std::filesystem::path;

class filesystem {
public:
  virtual file_status status(const path &p) const = 0;
};

} // namespace pfs

#endif