#pragma once

#include <map>
#include <memory>

#include "Solver.hpp"
#include "../expr/Expr.hpp"
#include "../arch/Arch.hpp"
#include "../loader/AddressSpace.hpp"

namespace naaz::state
{

class MapMemory
{
  public:
    enum UninitReadBehavior { RET_SYM, RET_ZERO, THROW_ERR };
    struct SymAccessBehavior {
        uint16_t max_n_eval_read, max_n_eval_write;
    };

  private:
    UninitReadBehavior                  m_uninit_behavior;
    SymAccessBehavior                   m_sym_access_behavior;
    loader::AddressSpace*               m_as;
    std::map<uint64_t, expr::BVExprPtr> m_memory;
    std::string                         m_name;
    Solver*                             m_solver = nullptr;

    expr::BVExprPtr read_byte(uint64_t addr);
    void            write_byte(uint64_t addr, expr::BVExprPtr value);

  public:
    MapMemory(const std::string& name, loader::AddressSpace* as,
              SymAccessBehavior  ab,
              UninitReadBehavior b = UninitReadBehavior::RET_SYM);
    MapMemory(const std::string& name, loader::AddressSpace* as,
              UninitReadBehavior b = UninitReadBehavior::RET_SYM)
        : m_name(name), m_as(as), m_uninit_behavior(b)
    {
        m_sym_access_behavior = {.max_n_eval_read  = 256,
                                 .max_n_eval_write = 64};
    }
    MapMemory(const std::string& name,
              UninitReadBehavior b = UninitReadBehavior::RET_SYM)
        : MapMemory(name, nullptr, b)
    {
    }
    MapMemory(const MapMemory& other)
        : m_name(other.m_name), m_memory(other.m_memory), m_as(other.m_as),
          m_uninit_behavior(other.m_uninit_behavior),
          m_sym_access_behavior(other.m_sym_access_behavior)
    {
    }

    void set_solver(Solver* s) { m_solver = s; }

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
