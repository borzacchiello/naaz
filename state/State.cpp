#include "State.hpp"

namespace naaz::state
{

State::State(const State& other)
    : m_as(other.m_as), m_lifter(other.m_lifter),
      m_constraints(other.m_constraints)
{
    m_ram  = other.m_ram->clone();
    m_regs = other.m_regs->clone();
}

expr::ExprPtr State::read(expr::ExprPtr addr, size_t len)
{
    return m_ram->read(addr, len, arch().endianess());
}

expr::ExprPtr State::read(uint64_t addr, size_t len)
{
    return m_ram->read(addr, len, arch().endianess());
}

void State::write(expr::ExprPtr addr, expr::ExprPtr data)
{
    m_ram->write(addr, data, arch().endianess());
}

void State::write(uint64_t addr, expr::ExprPtr data)
{
    m_ram->write(addr, data, arch().endianess());
}

expr::ExprPtr State::reg_read(const std::string& name)
{
    csleigh_Varnode reg_varnode = m_lifter->reg(name);
    return reg_read(reg_varnode.offset, reg_varnode.size);
}

expr::ExprPtr State::reg_read(uint64_t offset, size_t size)
{
    return m_regs->read(offset, size, Endianess::BIG);
}

void State::reg_write(const std::string& name, expr::ExprPtr data)
{
    csleigh_Varnode reg_varnode = m_lifter->reg(name);
    return reg_write(reg_varnode.offset, data);
}

void State::reg_write(uint64_t offset, expr::ExprPtr data)
{
    m_regs->write(offset, data, Endianess::BIG);
}

void State::add_constraint(expr::ExprPtr c) { m_constraints.insert(c); }

} // namespace naaz::state
