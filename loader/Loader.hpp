#pragma once

#include <memory>

#include "../arch/Arch.hpp"
#include "AddressSpace.hpp"

namespace naaz::loader
{

enum BinaryType { ELF, PE };
enum SyscallABI { LINUX_X86_64, LINUX_ARMv7, UNKNOWN };

class Loader
{
    virtual std::shared_ptr<AddressSpace> address_space()     = 0;
    virtual const Arch&                   arch() const        = 0;
    virtual BinaryType                    bin_type() const    = 0;
    virtual SyscallABI                    syscall_abi() const = 0;
    virtual uint64_t                      entrypoint() const  = 0;
    virtual state::StatePtr               entry_state() const = 0;
};

} // namespace naaz::loader
