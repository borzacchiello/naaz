#pragma once

#include <memory>

#include "../arch/Arch.hpp"
#include "../state/State.hpp"
#include "AddressSpace.hpp"

namespace naaz::loader
{

enum BinaryType { ELF, PE };

class Loader
{
    virtual std::shared_ptr<AddressSpace> address_space()     = 0;
    virtual const Arch&                   arch() const        = 0;
    virtual BinaryType                    bin_type() const    = 0;
    virtual uint64_t                      entrypoint() const  = 0;
    virtual state::StatePtr               entry_state() const = 0;
};

} // namespace naaz::loader
