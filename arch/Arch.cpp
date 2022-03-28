#include "Arch.hpp"

#include "../state/State.hpp"
#include "../models/Linker.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"

namespace naaz
{

std::filesystem::path Arch::getSleighProcessorDir()
{
    char* env_val = std::getenv("SLEIGH_PROCESSORS");
    if (env_val)
        return std::filesystem::path(env_val);

    // Change this
    return std::filesystem::path(
        "/home/luca/git/naaz/third_party/sleigh/processors");
}

void Arch::set_return(state::StatePtr s, uint64_t addr) const
{
    set_return(s, expr::ExprBuilder::The().mk_const(addr, ptr_size()));
}

namespace arch
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
    auto ret_addr = s->read(s->reg_read("RSP"), 8UL);
    if (ret_addr->kind() != expr::Expr::Kind::CONST) {
        err("arch::x86_64")
            << "handle_return(): FIXME, unhandled return to symbolic address"
            << std::endl;
        exit_fail();
    }

    s->set_pc(std::static_pointer_cast<const expr::ConstExpr>(ret_addr)
                  ->val()
                  .as_u64());
    o_successors.active.push_back(s);
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

void x86_64::set_int_param(CallConv cv, state::State& s, uint32_t i,
                           expr::BVExprPtr val) const
{
    switch (cv) {
        case CallConv::CDECL: {
            switch (i) {
                case 0:
                    s.reg_write("RDI", val);
                    return;
                case 1:
                    s.reg_write("RSI", val);
                case 2:
                    s.reg_write("RDX", val);
                case 3:
                    s.reg_write("RCX", val);
                case 4:
                    s.reg_write("R8", val);
                case 5:
                    s.reg_write("R9", val);
                default: {
                    uint64_t stack_off = (i + 1UL - 6UL) * 8UL;
                    auto     stack_off_expr =
                        expr::ExprBuilder::The().mk_const(stack_off, 64);
                    auto addr = expr::ExprBuilder::The().mk_add(
                        s.reg_read("RSP"), stack_off_expr);
                    return s.write(addr, val);
                }
            }
            break;
        }
        default:
            break;
    }

    err("arch::x86_64") << "set_int_param(): unsupported calling convention "
                        << cv << std::endl;
    exit_fail();
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
            ;
        }
        default:
            break;
    }
    err("arch::x86_64")
        << "get_return_int_value(): unsupported calling convention " << cv
        << std::endl;
    exit_fail();
}

} // namespace arch
} // namespace naaz
