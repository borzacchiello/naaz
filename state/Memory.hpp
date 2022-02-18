#pragma once

#include "../expr/Expr.hpp"
#include "../arch/Arch.hpp"

namespace naaz::memory
{

class Memory
{
  public:
    virtual expr::ExprPtr read(expr::ExprPtr addr, size_t len,
                               Endianess end = Endianess::LITTLE)  = 0;
    virtual expr::ExprPtr read(uint64_t addr, size_t len,
                               Endianess end = Endianess::LITTLE)  = 0;
    virtual void          write(expr::ExprPtr addr, expr::ExprPtr value,
                                Endianess end = Endianess::LITTLE) = 0;
    virtual void          write(uint64_t addr, expr::ExprPtr value,
                                Endianess end = Endianess::LITTLE) = 0;
};

} // namespace naaz::memory
