#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

void libc_start_main_exit_wrapper::exec(
    state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto retcode = s->arch().get_return_int_value(m_call_conv, *s);
    if (retcode->kind() != expr::Expr::Kind::CONST) {
        s->retcode = 0;
    } else {
        s->retcode = std::static_pointer_cast<const expr::ConstExpr>(retcode)
                         ->val()
                         .as_s64();
    }
    s->exited = true;
    o_successors.exited.push_back(s);
}

void __libc_start_main::exec(state::StatePtr           s,
                             executor::ExecutorResult& o_successors) const
{
    auto main_addr = s->get_int_param(m_call_conv, 0);
    if (main_addr->kind() != expr::Expr::Kind::CONST) {
        err("libc_start_main") << "main address is symbolic" << std::endl;
        exit_fail();
    }

    // The return address of libc_start_main is the exit function
    s->arch().set_return(s, s->get_libc_start_main_exit_wrapper_address());

    // Set argv (if defined)
    std::vector<expr::BVExprPtr> args;

    const auto& argv = s->get_argv();
    args.push_back(
        expr::ExprBuilder::The().mk_const(argv.size(), s->arch().ptr_size()));
    if (argv.size() > 0) {
        auto vec_size = argv.size() * (s->arch().ptr_size() / 8UL);
        auto vec_ptr  = s->allocate(vec_size);
        args.push_back(
            expr::ExprBuilder::The().mk_const(vec_ptr, s->arch().ptr_size()));

        uint32_t i = 0;
        for (auto arg : argv) {
            if (arg->size() % 8UL != 0) {
                err("__libc_start_main") << "invalid argv" << std::endl;
                exit_fail();
            }

            arg = expr::ExprBuilder::The().mk_concat(
                arg, expr::ExprBuilder::The().mk_const(0UL, 8));
            auto arg_ptr = s->allocate(arg->size() / 8UL);
            s->write_buf(arg_ptr, arg);
            s->write(vec_ptr + i * (s->arch().ptr_size() / 8UL),
                     expr::ExprBuilder::The().mk_const(arg_ptr,
                                                       s->arch().ptr_size()));
            i += 1;
        }
    }
    s->arch().set_int_params(m_call_conv, *s, args);

    auto main_addr_conc =
        std::static_pointer_cast<const expr::ConstExpr>(main_addr)
            ->val()
            .as_u64();
    s->set_pc(main_addr_conc);
    o_successors.active.push_back(s);
}

} // namespace naaz::models::libc
