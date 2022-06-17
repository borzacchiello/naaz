#include "arm32LE.hpp"

#include "../state/State.hpp"
#include "../models/Linker.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"

namespace naaz::arch
{

std::filesystem::path arm32LE::getSleighSLA() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("ARM/data/languages/ARM7_le.sla");
}

std::filesystem::path arm32LE::getSleighPSPEC() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("ARM/data/languages/ARMt.pspec");
}

const std::string& arm32LE::description() const
{
    static std::string descr = "ARM : 32-bit : LE";
    return descr;
}

void arm32LE::init_state(state::State& s) const
{
    s.reg_write("sp", expr::ExprBuilder::The().mk_const(stack_ptr, 32));
}

expr::BVExprPtr arm32LE::stack_pop(state::State& s) const
{
    auto ptr_size_off = expr::ExprBuilder::The().mk_const(4, 32);
    auto stack_val    = s.reg_read("sp");
    auto val          = s.read(stack_val, 4);
    stack_val = expr::ExprBuilder::The().mk_add(stack_val, ptr_size_off);
    s.reg_write("sp", stack_val);
    return val;
}

void arm32LE::stack_push(state::State& s, expr::BVExprPtr val) const
{
    if (val->size() != 32UL) {
        err("arm32LE") << "invalid stack_push" << std::endl;
        exit_fail();
    }

    auto ptr_size_off = expr::ExprBuilder::The().mk_const(4, 32);
    auto stack_val    = s.reg_read("sp");
    stack_val = expr::ExprBuilder::The().mk_sub(stack_val, ptr_size_off);
    s.write(stack_val, val);
    s.reg_write("sp", stack_val);
}

void arm32LE::set_return(state::StatePtr s, expr::BVExprPtr addr) const
{
    if (addr->size() != ptr_size()) {
        err("arch::arm32LE")
            << "set_return(): invalid return value" << std::endl;
        exit_fail();
    }

    s->write(s->reg_read("sp"), addr);
}

void arm32LE::handle_return(state::StatePtr           s,
                            executor::ExecutorResult& o_successors) const
{
    auto ret_addr = s->reg_read("lr");
    if (ret_addr->kind() != expr::Expr::Kind::CONST) {
        err("arch::arm32LE")
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

const std::string& arm32LE::get_stack_ptr_reg() const
{
    static std::string r = "sp";
    return r;
}

int arm32LE::get_syscall_num(state::State& s) const
{
    err("arch::arm32LE") << "get_syscall_param(): implement me" << std::endl;
    exit_fail();
}

expr::BVExprPtr arm32LE::get_syscall_param(state::State& s, uint32_t i) const
{
    err("arch::arm32LE") << "get_syscall_param(): implement me" << std::endl;
    exit_fail();
}

void arm32LE::set_syscall_return_value(state::State&   s,
                                       expr::BVExprPtr val) const
{
    err("arch::arm32LE") << "set_syscall_return_value(): implement me"
                         << std::endl;
    exit_fail();
}

expr::BVExprPtr arm32LE::get_int_param(CallConv cv, state::State& s,
                                       uint32_t i) const
{
    switch (cv) {
        case CallConv::CDECL: {
            switch (i) {
                case 0:
                    return s.reg_read("r0");
                case 1:
                    return s.reg_read("r1");
                case 2:
                    return s.reg_read("r2");
                case 3:
                    return s.reg_read("r3");
                default: {
                    uint64_t stack_off = (i + 1UL - 4UL) * 4UL;
                    auto     stack_off_expr =
                        expr::ExprBuilder::The().mk_const(stack_off, 32);
                    auto addr = expr::ExprBuilder::The().mk_add(
                        s.reg_read("sp"), stack_off_expr);
                    return s.read(addr, 4UL);
                }
            }
            break;
        }
        default:
            break;
    }

    err("arch::arm32LE") << "get_int_param(): unsupported calling convention "
                         << cv << std::endl;
    exit_fail();
}

void arm32LE::set_int_params(CallConv cv, state::State& s,
                             std::vector<expr::BVExprPtr> values) const
{
    int i = 0;
    for (auto val : values) {
        switch (cv) {
            case CallConv::CDECL: {
                switch (i++) {
                    case 0:
                        s.reg_write("r0", val);
                        break;
                    case 1:
                        s.reg_write("r1", val);
                        break;
                    case 2:
                        s.reg_write("r2", val);
                        break;
                    case 3:
                        s.reg_write("r3", val);
                        break;
                    default: {
                        stack_push(s, val);
                        break;
                    }
                }
                break;
            }
            default: {
                err("arch::arm32LE")
                    << "set_int_param(): unsupported calling convention " << cv
                    << std::endl;
                exit_fail();
            }
        }
    }
}

void arm32LE::set_return_int_value(CallConv cv, state::State& s,
                                   expr::BVExprPtr val) const
{
    switch (cv) {
        case CallConv::CDECL: {
            auto zext_val = expr::ExprBuilder::The().mk_zext(val, 32UL);
            s.reg_write("R0", zext_val);
            return;
        }
        default:
            break;
    }
    err("arch::arm32LE")
        << "set_return_int_value(): unsupported calling convention " << cv
        << std::endl;
    exit_fail();
}

expr::BVExprPtr arm32LE::get_return_int_value(CallConv      cv,
                                              state::State& s) const
{
    switch (cv) {
        case CallConv::CDECL: {
            return s.reg_read("r0");
        }
        default:
            break;
    }
    err("arch::arm32LE")
        << "get_return_int_value(): unsupported calling convention " << cv
        << std::endl;
    exit_fail();
}

} // namespace naaz::arch
