#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

void libc_start_main::exec(state::StatePtr           s,
                           executor::ExecutorResult& o_successors) const
{
    auto main_addr = s->get_int_param(m_call_conv, 0);
    if (main_addr->kind() != expr::Expr::Kind::CONST) {
        err("libc_start_main") << "main address is symbolic" << std::endl;
        exit_fail();
    }

    // The return address of libc_start_main is the exit function
    s->arch().set_return(s, s->get_exit_address());

    auto main_addr_conc =
        std::static_pointer_cast<const expr::ConstExpr>(main_addr)
            ->val()
            .as_u64();
    s->set_pc(main_addr_conc);
    o_successors.active.push_back(s);
}

} // namespace naaz::models::libc
