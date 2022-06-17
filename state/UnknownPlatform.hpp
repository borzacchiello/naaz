#pragma once

#include "Platform.hpp"

namespace naaz::state
{

class UnknownPlatform : public Platform
{
  public:
    virtual const std::string& name() const
    {
        static const std::string n = "unknown";
        return n;
    };
    virtual const models::SyscallModel& syscall(int num) const;
    virtual loader::SyscallABI          abi() const
    {
        return loader::SyscallABI::UNKNOWN;
    }
};

} // namespace naaz::state
