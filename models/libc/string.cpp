#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"

namespace naaz::models::libc
{

void memcpy::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto dst  = s->get_int_param(m_call_conv, 0);
    auto src  = s->get_int_param(m_call_conv, 1);
    auto size = s->get_int_param(m_call_conv, 2);

    if (dst->kind() != expr::Expr::Kind::CONST) {
        err("memcpy") << "dst is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (src->kind() != expr::Expr::Kind::CONST) {
        err("memcpy") << "src is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("memcpy") << "size is symbolic (FIXME)" << std::endl;
        exit_fail();
    }

    auto dst_ =
        std::static_pointer_cast<const expr::ConstExpr>(dst)->val().as_u64();
    auto src_ =
        std::static_pointer_cast<const expr::ConstExpr>(src)->val().as_u64();
    auto size_ =
        std::static_pointer_cast<const expr::ConstExpr>(size)->val().as_u64();

    s->write_buf(dst_, s->read(src_, size_));
    s->arch().handle_return(s, o_successors);
}

void memcmp::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto buf1 = s->get_int_param(m_call_conv, 0);
    auto buf2 = s->get_int_param(m_call_conv, 1);
    auto size = s->get_int_param(m_call_conv, 2);

    if (buf1->kind() != expr::Expr::Kind::CONST) {
        err("memcmp") << "dst is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (buf2->kind() != expr::Expr::Kind::CONST) {
        err("memcmp") << "src is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("memcmp") << "size is symbolic (FIXME)" << std::endl;
        exit_fail();
    }

    auto buf1_ =
        std::static_pointer_cast<const expr::ConstExpr>(buf1)->val().as_u64();
    auto buf2_ =
        std::static_pointer_cast<const expr::ConstExpr>(buf2)->val().as_u64();
    auto size_ =
        std::static_pointer_cast<const expr::ConstExpr>(size)->val().as_u64();

    auto expr = expr::ExprBuilder::The().mk_eq(s->read_buf(buf1_, size_),
                                               s->read_buf(buf2_, size_));
    // FIXME: this is wrong, the semantics of memcmp is slightly different
    auto ret = expr::ExprBuilder::The().mk_ite(
        expr, expr::ExprBuilder::The().mk_const(0, 8),
        expr::ExprBuilder::The().mk_const(1, 8));
    s->arch().set_return_int_value(m_call_conv, *s, ret);
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
