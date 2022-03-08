#pragma once

#include <string>
#include <memory>

#include "MapMemory.hpp"

namespace naaz::state
{

class FileHandle;
class File
{
    std::string                m_filename;
    size_t                     m_size;
    std::unique_ptr<MapMemory> m_content;

  public:
    File(const std::string& filename);
    File(const File& other);

    void            enlarge(uint64_t off);
    expr::BVExprPtr read(uint64_t off, size_t size);
    expr::BVExprPtr read_all();
    void            write(uint64_t off, expr::BVExprPtr data);

    size_t             size() const { return m_size; }
    const std::string& name() const { return m_filename; }

    FileHandle gen_handle(int fd);

    std::unique_ptr<File> clone() const;
};

class FileHandle
{
    File&    m_file;
    uint64_t m_off;
    int      m_descriptor;

    FileHandle(File& f, int fd) : m_file(f), m_descriptor(fd), m_off(0) {}

  public:
    void            seek(uint64_t off);
    expr::BVExprPtr read(size_t size);
    void            write(expr::BVExprPtr data);

    int      fd() const { return m_descriptor; }
    uint64_t off() const { return m_off; }
    File&    file() { return m_file; }

    friend class File;
};

} // namespace naaz::state
