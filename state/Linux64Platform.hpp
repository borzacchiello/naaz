#pragma once

#include "Platform.hpp"

namespace naaz::state
{

class Linux64Platform : public Platform
{
  private:
    static const models::SyscallModel* syscalls[314];

    loader::SyscallABI m_abi;

  public:
    Linux64Platform(loader::SyscallABI abi) : m_abi(abi) {}

    virtual const std::string& name() const
    {
        static const std::string n = "linux_64";
        return n;
    }
    virtual const models::SyscallModel& syscall(int num) const;
    virtual loader::SyscallABI          abi() const { return m_abi; }
};

} // namespace naaz::state
