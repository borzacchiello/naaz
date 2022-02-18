#pragma once

#include <map>
#include <memory>

#include "../expr/Expr.hpp"
#include "../arch/Arch.hpp"
#include "../loader/AddressSpace.hpp"

namespace naaz::state
{

class MapMemory
{
    loader::AddressSpace*             m_as;
    std::map<uint64_t, expr::ExprPtr> m_memory;

    expr::ExprPtr read_byte(uint64_t addr);
    void          write_byte(uint64_t addr, expr::ExprPtr value);

  public:
    MapMemory() : m_as(nullptr) {}
    MapMemory(loader::AddressSpace* as) : m_as(as) {}
    MapMemory(const MapMemory& other)
        : m_memory(other.m_memory), m_as(other.m_as)
    {
    }

    expr::ExprPtr read(expr::ExprPtr addr, size_t len,
                       Endianess end = Endianess::LITTLE);
    expr::ExprPtr read(uint64_t addr, size_t len,
                       Endianess end = Endianess::LITTLE);
    void          write(expr::ExprPtr addr, expr::ExprPtr value,
                        Endianess end = Endianess::LITTLE);
    void          write(uint64_t addr, expr::ExprPtr value,
                        Endianess end = Endianess::LITTLE);

    std::unique_ptr<MapMemory> clone();
};

} // namespace naaz::state
