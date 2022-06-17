#pragma once

#include <string>

#include "../models/SyscallModel.hpp"
#include "../loader/Loader.hpp"

namespace naaz::state
{

class Platform
{
  public:
    virtual const std::string&          name() const           = 0;
    virtual const models::SyscallModel& syscall(int num) const = 0;
    virtual loader::SyscallABI          abi() const            = 0;
};

} // namespace naaz::state
