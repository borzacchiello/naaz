#include "x86_64.hpp"

#include "../state/State.hpp"
#include "../models/Linker.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"

namespace naaz::arch
{

std::filesystem::path x86_64::getSleighSLA() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("x86/data/languages/x86-64.sla");
}

std::filesystem::path x86_64::getSleighPSPEC() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("x86/data/languages/x86-64.pspec");
}

const std::string& x86_64::description() const
{
    static std::string descr = "x86_64 : 64-bit : LE";
    return descr;
}

void x86_64::init_state(state::State& s) const
{
    s.reg_write("RSP", expr::ExprBuilder::The().mk_const(stack_ptr, 64));
    s.reg_write("FS_OFFSET",
                expr::ExprBuilder::The().mk_const(fs_base_ptr, 64));

    s.reg_write("PF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("AF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("ZF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("SF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("IF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("DF", expr::ExprBuilder::The().mk_const(0UL, 8));
    s.reg_write("OF", expr::ExprBuilder::The().mk_const(0UL, 8));
}

expr::BVExprPtr x86_64::stack_pop(state::State& s) const
{
    auto ptr_size_off = expr::ExprBuilder::The().mk_const(8, 64);
    auto stack_val    = s.reg_read("RSP");
    auto val          = s.read(stack_val, 8);
    stack_val = expr::ExprBuilder::The().mk_add(stack_val, ptr_size_off);
    s.reg_write("RSP", stack_val);
    return val;
}

void x86_64::stack_push(state::State& s, expr::BVExprPtr val) const
{
    if (!val->size() != 64UL) {
        err("x86_64") << "invalid stack_push" << std::endl;
        exit_fail();
    }

    auto ptr_size_off = expr::ExprBuilder::The().mk_const(8, 64);
    auto stack_val    = s.reg_read("RSP");
    stack_val = expr::ExprBuilder::The().mk_sub(stack_val, ptr_size_off);
    s.write(stack_val, val);
    s.reg_write("RSP", stack_val);
}

void x86_64::set_return(state::StatePtr s, expr::BVExprPtr addr) const
{
    if (addr->size() != ptr_size()) {
        err("arch::x86_64")
            << "set_return(): invalid return value" << std::endl;
        exit_fail();
    }

    s->write(s->reg_read("RSP"), addr);
}

void x86_64::handle_return(state::StatePtr           s,
                           executor::ExecutorResult& o_successors) const
{
    auto ret_addr = stack_pop(s);
    if (ret_addr->kind() != expr::Expr::Kind::CONST) {
        err("arch::x86_64")
            << "handle_return(): FIXME, unhandled return to symbolic address"
            << std::endl;
        exit_fail();
    }

    s->register_ret();
    s->set_pc(std::static_pointer_cast<const expr::ConstExpr>(ret_addr)
                  ->val()
                  .as_u64());
    o_successors.active.push_back(s);
}

const std::string& x86_64::get_stack_ptr_reg() const
{
    static std::string r = "RSP";
    return r;
}

expr::BVExprPtr x86_64::get_int_param(CallConv cv, state::State& s,
                                      uint32_t i) const
{
    switch (cv) {
        case CallConv::CDECL: {
            switch (i) {
                case 0:
                    return s.reg_read("RDI");
                case 1:
                    return s.reg_read("RSI");
                case 2:
                    return s.reg_read("RDX");
                case 3:
                    return s.reg_read("RCX");
                case 4:
                    return s.reg_read("R8");
                case 5:
                    return s.reg_read("R9");
                default: {
                    uint64_t stack_off = (i + 1UL - 6UL) * 8UL;
                    auto     stack_off_expr =
                        expr::ExprBuilder::The().mk_const(stack_off, 64);
                    auto addr = expr::ExprBuilder::The().mk_add(
                        s.reg_read("RSP"), stack_off_expr);
                    return s.read(addr, 8UL);
                }
            }
            break;
        }
        default:
            break;
    }

    err("arch::x86_64") << "get_int_param(): unsupported calling convention "
                        << cv << std::endl;
    exit_fail();
}

void x86_64::set_int_params(CallConv cv, state::State& s,
                            std::vector<expr::BVExprPtr> values) const
{
    int i = 0;
    for (auto val : values) {
        switch (cv) {
            case CallConv::CDECL: {
                switch (i++) {
                    case 0:
                        s.reg_write("RDI", val);
                        break;
                    case 1:
                        s.reg_write("RSI", val);
                        break;
                    case 2:
                        s.reg_write("RDX", val);
                        break;
                    case 3:
                        s.reg_write("RCX", val);
                        break;
                    case 4:
                        s.reg_write("R8", val);
                        break;
                    case 5:
                        s.reg_write("R9", val);
                        break;
                    default: {
                        stack_push(s, val);
                        break;
                    }
                }
                break;
            }
            default: {
                err("arch::x86_64")
                    << "set_int_param(): unsupported calling convention " << cv
                    << std::endl;
                exit_fail();
            }
        }
    }
}

void x86_64::set_return_int_value(CallConv cv, state::State& s,
                                  expr::BVExprPtr val) const
{
    switch (cv) {
        case CallConv::CDECL: {
            auto zext_val = expr::ExprBuilder::The().mk_zext(val, 64UL);
            s.reg_write("RAX", zext_val);
            return;
        }
        default:
            break;
    }
    err("arch::x86_64")
        << "set_return_int_value(): unsupported calling convention " << cv
        << std::endl;
    exit_fail();
}

expr::BVExprPtr x86_64::get_return_int_value(CallConv cv, state::State& s) const
{
    switch (cv) {
        case CallConv::CDECL: {
            return s.reg_read("RAX");
        }
        default:
            break;
    }
    err("arch::x86_64")
        << "get_return_int_value(): unsupported calling convention " << cv
        << std::endl;
    exit_fail();
}

} // namespace naaz::arch
