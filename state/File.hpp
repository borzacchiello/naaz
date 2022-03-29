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
    std::string m_filename;
    uint64_t    m_off;
    int         m_descriptor;

    FileHandle(const std::string& filename, int fd)
        : m_filename(filename), m_descriptor(fd), m_off(0)
    {
    }

  public:
    void            seek(File& file, uint64_t off);
    expr::BVExprPtr read(File& file, size_t size);
    void            write(File& file, expr::BVExprPtr data);

    int                fd() const { return m_descriptor; }
    uint64_t           off() const { return m_off; }
    const std::string& filename() const { return m_filename; }

    friend class File;
};

} // namespace naaz::state
