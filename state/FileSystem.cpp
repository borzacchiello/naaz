#include <cassert>

#include "FileSystem.hpp"

#include "../util/ioutil.hpp"

namespace naaz::state
{

FileSystem::FileSystem()
{
    m_free_fd = 0;

    int fd;
    fd = open("stdin");
    assert(fd == 0 && "FileSystem(): unexpected stdin fd");

    fd = open("stdout");
    assert(fd == 1 && "FileSystem(): unexpected stdout fd");

    fd = open("stderr");
    assert(fd == 2 && "FileSystem(): unexpected stderr fd");
}

FileSystem::FileSystem(const FileSystem& other)
    : m_open_files(other.m_open_files), m_free_fd(other.m_free_fd)
{
    for (const auto& pair : other.m_files)
        m_files[pair.first] = pair.second->clone();
}

int FileSystem::open(const std::string& filepath)
{
    if (!m_files.contains(filepath))
        m_files[filepath] = std::unique_ptr<File>(new File(filepath));

    FileHandle h = m_files[filepath]->gen_handle(m_free_fd++);
    m_open_files.emplace(h.fd(), h);
    return h.fd();
}

void FileSystem::close(int fd)
{
    if (!m_open_files.contains(fd)) {
        err("FileSystem") << "close(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    m_open_files.erase(fd);
    if (fd == m_free_fd - 1)
        m_free_fd--;
}

void FileSystem::seek(int fd, uint64_t off)
{
    if (!m_open_files.contains(fd)) {
        err("FileSystem") << "seek(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    auto& handle = m_open_files.at(fd);
    handle.seek(*m_files.at(handle.filename()), off);
}

expr::BVExprPtr FileSystem::read(int fd, ssize_t size)
{
    if (!m_open_files.contains(fd)) {
        err("FileSystem") << "read(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    auto& handle = m_open_files.at(fd);
    return handle.read(*m_files.at(handle.filename()), size);
}

void FileSystem::write(int fd, expr::BVExprPtr data)
{
    if (!m_open_files.contains(fd)) {
        err("FileSystem") << "write(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    auto& handle = m_open_files.at(fd);
    handle.write(*m_files.at(handle.filename()), data);
}

std::unique_ptr<FileSystem> FileSystem::clone() const
{
    return std::unique_ptr<FileSystem>(new FileSystem(*this));
}

std::vector<File*> FileSystem::files()
{
    std::vector<File*> res;
    for (auto const& [name, file] : m_files) {
        res.push_back(file.get());
    }
    return res;
}

} // namespace naaz::state
