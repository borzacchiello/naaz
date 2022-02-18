#include "MapMemory.hpp"

#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"
#include "../util/strutil.hpp"

namespace naaz::state
{

using namespace naaz::expr;

ExprPtr MapMemory::read(ExprPtr addr, size_t len, Endianess end)
{
    if (addr->kind() != Expr::Kind::CONST) {
        // FIXME: implement this
        err("MapMemory")
            << "read(): symbolic memory accesses are not implemented"
            << std::endl;
        exit_fail();
    }

    ConstExprPtr addr_ = std::static_pointer_cast<const ConstExpr>(addr);
    return read(addr_->val(), len, end);
}

ExprPtr MapMemory::read_byte(uint64_t addr)
{
    if (!m_memory.contains(addr)) {
        if (m_as) {
            auto b = m_as->read_byte(addr);
            if (b.has_value()) {
                // The value is in the AddressSpace
                ExprPtr c = expr::ExprBuilder::The().mk_const(b.value(), 8);
                write_byte(addr, c);
                return c;
            }
        }

        SymExprPtr sym =
            ExprBuilder::The().mk_sym(string_format("mem_0x%lx", addr), 8);
        write_byte(addr, sym);
        return sym;
    }

    return m_memory[addr];
}

ExprPtr MapMemory::read(uint64_t addr, size_t len, Endianess end)
{
    if (len == 0) {
        err("MapMemory") << "read(): zero len" << std::endl;
        exit_fail();
    }

    ExprPtr res = read_byte(addr);
    for (size_t i = 1; i < len; ++i) {
        if (end == Endianess::LITTLE)
            res = ExprBuilder::The().mk_concat(res, read_byte(addr + i));
        else
            res = ExprBuilder::The().mk_concat(read_byte(addr + i), res);
    }
    return res;
}

void MapMemory::write(ExprPtr addr, ExprPtr value, Endianess end)
{
    if (addr->kind() != Expr::Kind::CONST) {
        // FIXME: implement this
        err("MapMemory") << "write(): symbolic memory accesses are "
                            "not implemented"
                         << std::endl;
        exit_fail();
    }

    ConstExprPtr addr_ = std::static_pointer_cast<const ConstExpr>(addr);
    return write(addr_->val(), value, end);
}

void MapMemory::write_byte(uint64_t addr, ExprPtr value)
{
    if (value->size() != 8) {
        err("MapMemory")
            << "write_byte(): the functions expects 8-bit values only"
            << std::endl;
        exit_fail();
    }

    m_memory[addr] = value;
}

void MapMemory::write(uint64_t addr, ExprPtr value, Endianess end)
{
    size_t len = value->size();
    if (len % 8 != 0) {
        err("MapMemory") << "write(): the value size is not a multiple of 8"
                         << std::endl;
        exit_fail();
    }

    len = len / 8UL;
    for (size_t i = 0; i < len; ++i) {
        ExprPtr e =
            end == Endianess::LITTLE
                ? ExprBuilder::The().mk_extract(value, (len - i - 1) * 8 + 7,
                                                (len - i - 1) * 8)
                : ExprBuilder::The().mk_extract(value, i * 8 + 7, i * 8);
        write_byte(addr + i, e);
    }
}

std::unique_ptr<MapMemory> MapMemory::clone()
{
    return std::unique_ptr<MapMemory>(new MapMemory(*this));
}

} // namespace naaz::state
