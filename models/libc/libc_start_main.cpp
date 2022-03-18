#include "directory.hpp"

#include "../../expr/Expr.hpp"
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

    auto main_addr_conc =
        std::static_pointer_cast<const expr::ConstExpr>(main_addr)
            ->val()
            .as_u64();
    s->set_pc(main_addr_conc);
    o_successors.active.push_back(s);
}

} // namespace naaz::models::libc
