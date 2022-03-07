#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

void exit::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto retcode = s->get_int_param(m_call_conv, 0);
    if (retcode->kind() != expr::Expr::Kind::CONST) {
        err("exit") << "retcode is symbolic" << std::endl;
        exit_fail();
    }

    s->retcode = std::static_pointer_cast<const expr::ConstExpr>(retcode)
                     ->val()
                     .as_s64();
    s->exited = true;
    o_successors.exited.push_back(s);
}

} // namespace naaz::models::libc
