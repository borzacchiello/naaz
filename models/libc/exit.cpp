#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

void exit::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto retcode = expr::ExprBuilder::The().mk_extract(
        s->get_int_param(m_call_conv, 0), 31, 0);

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

void abort::exec(state::StatePtr           s,
                 executor::ExecutorResult& o_successors) const
{
    s->retcode = 0xffffffff;
    s->exited  = true;
    o_successors.exited.push_back(s);
}

} // namespace naaz::models::libc
