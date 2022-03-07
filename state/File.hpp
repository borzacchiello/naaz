#pragma once

#include <string>
#include <memory>

#include "MapMemory.hpp"

namespace naaz::state
{

class File
{
    std::string                m_filename;
    int                        m_descriptor;
    uint64_t                   m_off;
    size_t                     m_size;
    std::unique_ptr<MapMemory> m_content;

  public:
    File(const std::string& filename, int fd);
    File(const File& other);

    void            seek(uint64_t off);
    expr::BVExprPtr read(size_t size);
    void            write(expr::BVExprPtr data);

    size_t size() const { return m_size; }
    int    fd() const { return m_descriptor; }

    std::unique_ptr<File> clone() const;
};

} // namespace naaz::state
