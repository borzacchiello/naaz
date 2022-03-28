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

void strlen::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto str = s->get_int_param(m_call_conv, 0);

    if (str->kind() != expr::Expr::Kind::CONST) {
        err("strlen") << "str is symbolic" << std::endl;
        exit_fail();
    }

    int  max_forks = 32;
    auto curr =
        std::static_pointer_cast<const expr::ConstExpr>(str)->val().as_u64();
    auto start = curr;
    while (1) {
        auto b = s->read(curr, 1);

        if (b->kind() == expr::Expr::Kind::CONST) {
            auto b_ = std::static_pointer_cast<const expr::ConstExpr>(b);
            if (b_->val().is_zero())
                break;
        } else {
            auto is_zero_expr = expr::ExprBuilder::The().mk_eq(
                b, expr::ExprBuilder::The().mk_const(0UL, 8));
            if (s->solver().may_be_true(is_zero_expr) ==
                solver::CheckResult::SAT) {
                state::StatePtr succ = max_forks <= 0 ? s : s->clone();
                succ->solver().add(is_zero_expr);
                succ->write(curr, expr::ExprBuilder::The().mk_const(0UL, 8));
                s->arch().set_return_int_value(
                    m_call_conv, *succ,
                    expr::ExprBuilder::The().mk_const(curr - start,
                                                      s->arch().ptr_size()));
                s->arch().handle_return(succ, o_successors);

                if (max_forks <= 0)
                    return;
                s->solver().add(expr::ExprBuilder::The().mk_not(is_zero_expr));
                max_forks--;
            } else {
                // is symbolic but can only be zero
                s->write(curr, expr::ExprBuilder::The().mk_const(0UL, 8));
                break;
            }
        }
        curr += 1;
    }

    s->arch().set_return_int_value(
        m_call_conv, *s,
        expr::ExprBuilder::The().mk_const(curr - start, s->arch().ptr_size()));
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
