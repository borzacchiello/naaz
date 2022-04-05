#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

void ptrace::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    // just a stub that returns zero...
    s->arch().set_return_int_value(m_call_conv, *s,
                                   expr::ExprBuilder::The().mk_const(0UL, 8));
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
