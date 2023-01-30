#ifndef INCLUDED_PFS_STD_FILESYSTEM_HPP
#define INCLUDED_PFS_STD_FILESYSTEM_HPP

#include <pfs/filesystem.hpp>

namespace pfs {

class std_filesystem final : public filesystem {
public:
  file_status status(const path &p) const override {
    return std::filesystem::status(p);
  }
};

} // namespace pfs

#endif