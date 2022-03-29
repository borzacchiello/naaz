#include "File.hpp"
#include "../arch/Arch.hpp"

namespace naaz::state
{

File::File(const std::string& filename) : m_filename(filename), m_size(0)
{
    m_content = std::unique_ptr<MapMemory>(new MapMemory(filename));
}

File::File(const File& other)
    : m_filename(other.m_filename), m_size(other.m_size)
{
    m_content = other.m_content->clone();
}

void File::enlarge(uint64_t off)
{
    if (off > m_size)
        m_size = off;
}

expr::BVExprPtr File::read(uint64_t off, size_t size)
{
    auto res = m_content->read(off, size, Endianess::BIG);
    off += size;
    if (off > m_size)
        m_size = off;
    return res;
}

expr::BVExprPtr File::read_all()
{
    return m_content->read(0, m_size, Endianess::BIG);
}

void File::write(uint64_t off, expr::BVExprPtr data)
{
    m_content->write(off, data, Endianess::BIG);
    off += data->size() / 8UL;
    if (off > m_size)
        m_size = off;
}

std::unique_ptr<File> File::clone() const
{
    return std::unique_ptr<File>(new File(*this));
}

FileHandle File::gen_handle(int fd) { return FileHandle(m_filename, fd); }

void FileHandle::seek(File& file, uint64_t off)
{
    m_off = off;
    file.enlarge(off);
}

expr::BVExprPtr FileHandle::read(File& file, size_t size)
{
    auto res = file.read(m_off, size);
    m_off += size;
    return res;
}

void FileHandle::write(File& file, expr::BVExprPtr data)
{
    file.write(m_off, data);
    m_off += data->size() / 8UL;
}

} // namespace naaz::state
