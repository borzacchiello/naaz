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
}

void x86_64::handle_return(state::StatePtr           s,
                           executor::ExecutorResult& o_result) const
{
    auto ret_addr = s->read(s->reg_read("RSP"), 8UL);
    std::cerr << "returning to " << ret_addr->to_string() << std::endl;
    if (ret_addr->kind() != expr::Expr::Kind::CONST) {
        // FIXME: eval upto
        o_result.exited.push_back(s);
        return;
    }

    auto ret_addr_ = std::static_pointer_cast<const expr::ConstExpr>(ret_addr);
    s->set_pc(ret_addr_->val().as_u64());
    o_result.active.push_back(s);
    return;
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

} // namespace arch
} // namespace naaz
