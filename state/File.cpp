#include "File.hpp"
#include "../arch/Arch.hpp"

namespace naaz::state
{

File::File(const std::string& filename, int fd)
    : m_filename(filename), m_descriptor(fd), m_off(0), m_size(0)
{
    m_content = std::unique_ptr<MapMemory>(new MapMemory(filename));
}

File::File(const File& other)
    : m_filename(other.m_filename), m_descriptor(other.m_descriptor),
      m_off(other.m_off), m_size(other.m_size)
{
    m_content = other.m_content->clone();
}

void File::seek(uint64_t off)
{
    m_off = off;
    if (m_off > m_size)
        m_size = m_off;
}

expr::BVExprPtr File::read(size_t size)
{
    auto res = m_content->read(m_off, size, Endianess::BIG);
    m_off += size;
    if (m_off > m_size)
        m_size = m_off;
    return res;
}

void File::write(expr::BVExprPtr data)
{
    m_content->write(m_off, data, Endianess::BIG);
    m_off += data->size() / 8UL;
    if (m_off > m_size)
        m_size = m_off;
}

std::unique_ptr<File> File::clone() const
{
    return std::unique_ptr<File>(new File(*this));
}

} // namespace naaz::state
