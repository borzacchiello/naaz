#pragma once

#include <map>

#include "Memory.hpp"

namespace naaz::memory
{

class MapMemory : public Memory
{
    std::map<uint64_t, expr::ExprPtr> m_memory;

    virtual expr::ExprPtr read_byte(uint64_t addr);
    virtual void          write_byte(uint64_t addr, expr::ExprPtr value);

  public:
    virtual expr::ExprPtr read(expr::ExprPtr addr, size_t len,
                               Endianess end = Endianess::LITTLE);
    virtual expr::ExprPtr read(uint64_t addr, size_t len,
                               Endianess end = Endianess::LITTLE);
    virtual void          write(expr::ExprPtr addr, expr::ExprPtr value,
                                Endianess end = Endianess::LITTLE);
    virtual void          write(uint64_t addr, expr::ExprPtr value,
                                Endianess end = Endianess::LITTLE);
};

} // namespace naaz::memory
