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
  public:
    enum UninitReadBehavior { RET_SYM, RET_ZERO, THROW_ERR };

  private:
    UninitReadBehavior                  m_uninit_behavior;
    loader::AddressSpace*               m_as;
    std::map<uint64_t, expr::BVExprPtr> m_memory;
    std::string                         m_name;

    expr::BVExprPtr read_byte(uint64_t addr);
    void            write_byte(uint64_t addr, expr::BVExprPtr value);

  public:
    MapMemory(const std::string& name,
              UninitReadBehavior b = UninitReadBehavior::RET_SYM)
        : m_name(name), m_as(nullptr), m_uninit_behavior(b)
    {
    }
    MapMemory(const std::string& name, loader::AddressSpace* as,
              UninitReadBehavior b = UninitReadBehavior::RET_SYM)
        : m_name(name), m_as(as), m_uninit_behavior(b)
    {
    }
    MapMemory(const MapMemory& other)
        : m_name(other.m_name), m_memory(other.m_memory), m_as(other.m_as),
          m_uninit_behavior(other.m_uninit_behavior)
    {
    }

    expr::BVExprPtr read(expr::BVExprPtr addr, size_t len,
                         Endianess end = Endianess::LITTLE);
    expr::BVExprPtr read(uint64_t addr, size_t len,
                         Endianess end = Endianess::LITTLE);
    void            write(expr::BVExprPtr addr, expr::BVExprPtr value,
                          Endianess end = Endianess::LITTLE);
    void            write(uint64_t addr, expr::BVExprPtr value,
                          Endianess end = Endianess::LITTLE);

    std::unique_ptr<MapMemory> clone();
};

} // namespace naaz::state
