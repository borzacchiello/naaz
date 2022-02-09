#pragma once

#include "../arch/Arch.hpp"
#include "AddressSpace.hpp"

namespace naaz::loader
{

enum BinaryType { ELF, PE };

class Loader
{
    virtual AddressSpace& address_space()    = 0;
    virtual const Arch&   arch() const       = 0;
    virtual BinaryType    bin_type() const   = 0;
    virtual uint64_t      entrypoint() const = 0;
};

} // namespace naaz::loader
