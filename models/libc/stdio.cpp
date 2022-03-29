#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"
#include "string_utils.hpp"

namespace naaz::models::libc
{

void puts::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto str = s->get_int_param(m_call_conv, 0);

    if (str->kind() != expr::Expr::Kind::CONST) {
        err("puts") << "str pointer is symbolic" << std::endl;
        exit_fail();
    }

    auto str_addr =
        std::static_pointer_cast<const expr::ConstExpr>(str)->val().as_u64();
    auto resolved_strings = resolve_string(s, str_addr, 0, 256);
    if (resolved_strings.size() != 1) {
        err("puts") << "unable to resolve the string" << std::endl;
        exit_fail();
    }

    auto data = resolved_strings.at(0).str;
    s->fs().write(1, data);
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
