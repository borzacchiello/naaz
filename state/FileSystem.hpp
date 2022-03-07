#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <map>

#include "File.hpp"
#include "../expr/Expr.hpp"

namespace naaz::state
{

class FileSystem
{
    std::map<int, std::unique_ptr<File>> m_files;
    std::map<std::string, int>           m_filepath_to_fd;
    int                                  m_free_fd;

  public:
    FileSystem();
    FileSystem(const FileSystem& other);

    // posix-like file handling
    int             open(const std::string& filepath);
    void            close(int fd);
    void            seek(int fd, uint64_t off);
    expr::BVExprPtr read(int fd, ssize_t size);
    void            write(int fd, expr::BVExprPtr data);

    // other
    std::unique_ptr<FileSystem> clone() const;
};

}; // namespace naaz::state
