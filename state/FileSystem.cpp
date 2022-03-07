#include "FileSystem.hpp"

#include "../util/ioutil.hpp"

namespace naaz::state
{

FileSystem::FileSystem()
{
    m_files[0] = std::unique_ptr<File>(new File("stdin", 0));
    m_files[1] = std::unique_ptr<File>(new File("stdout", 1));
    m_files[2] = std::unique_ptr<File>(new File("stderr", 2));

    m_filepath_to_fd["stdin"]  = m_files[0]->fd();
    m_filepath_to_fd["stdout"] = m_files[1]->fd();
    m_filepath_to_fd["stderr"] = m_files[2]->fd();

    m_free_fd = 3;
}

FileSystem::FileSystem(const FileSystem& other)
    : m_filepath_to_fd(other.m_filepath_to_fd), m_free_fd(other.m_free_fd)
{
    for (const auto& pair : other.m_files)
        m_files[pair.first] = pair.second->clone();
}

int FileSystem::open(const std::string& filepath)
{
    if (m_filepath_to_fd.contains(filepath))
        return m_filepath_to_fd.at(filepath);

    int fd                     = m_free_fd++;
    m_files[fd]                = std::unique_ptr<File>(new File(filepath, fd));
    m_filepath_to_fd[filepath] = fd;
    return fd;
}

void FileSystem::close(int fd)
{
    // do nothing...
}

void FileSystem::seek(int fd, uint64_t off)
{
    if (!m_files.contains(fd)) {
        err("FileSystem") << "seek(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    m_files[fd]->seek(off);
}

expr::BVExprPtr FileSystem::read(int fd, ssize_t size)
{
    if (!m_files.contains(fd)) {
        err("FileSystem") << "read(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    return m_files[fd]->read(size);
}

void FileSystem::write(int fd, expr::BVExprPtr data)
{
    if (!m_files.contains(fd)) {
        err("FileSystem") << "write(): unknown descriptor " << fd << std::endl;
        exit_fail();
    }

    return m_files[fd]->write(data);
}

std::unique_ptr<FileSystem> FileSystem::clone() const
{
    return std::unique_ptr<FileSystem>(new FileSystem(*this));
}

std::vector<File*> FileSystem::files()
{
    std::vector<File*> res;
    for (auto const& [fd, file] : m_files) {
        res.push_back(file.get());
    }
    return res;
}

} // namespace naaz::state
