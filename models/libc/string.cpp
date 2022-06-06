#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"
#include "string_utils.hpp"

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

void strcmp::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    // FIXME: the semantics of strcmp is different

    auto str1 = s->get_int_param(m_call_conv, 0);
    auto str2 = s->get_int_param(m_call_conv, 1);

    if (str1->kind() != expr::Expr::Kind::CONST) {
        err("strcmp") << "str1 is symbolic" << std::endl;
        exit_fail();
    }

    if (str2->kind() != expr::Expr::Kind::CONST) {
        err("strcmp") << "str2 is symbolic" << std::endl;
        exit_fail();
    }

    auto str1_ =
        std::static_pointer_cast<const expr::ConstExpr>(str1)->val().as_u64();
    auto str2_ =
        std::static_pointer_cast<const expr::ConstExpr>(str2)->val().as_u64();

    int               max_size = 256;
    expr::BoolExprPtr res_expr = expr::ExprBuilder::The().mk_true();
    while (max_size-- > 0) {
        auto b1 = s->read(str1_, 1);
        auto b2 = s->read(str2_, 1);

        auto c = expr::ExprBuilder::The().mk_eq(b1, b2);
        if (c->kind() == expr::Expr::Kind::BOOL_CONST) {
            auto c_ = std::static_pointer_cast<const expr::BoolConst>(c);
            if (!c_->is_true()) {
                s->arch().set_return_int_value(
                    m_call_conv, *s, expr::ExprBuilder::The().mk_const(1, 8));
                s->arch().handle_return(s, o_successors);
                return;
            }
        }

        res_expr = expr::ExprBuilder::The().mk_bool_and(res_expr, c);

        if (b1->kind() == expr::Expr::Kind::CONST) {
            auto b1_ = std::static_pointer_cast<const expr::ConstExpr>(b1);
            if (b1_->val().as_u64() == 0)
                break;
        }

        if (b2->kind() == expr::Expr::Kind::CONST) {
            auto b2_ = std::static_pointer_cast<const expr::ConstExpr>(b2);
            if (b2_->val().as_u64() == 0)
                break;
        }

        str1_++;
        str2_++;
    }

    s->arch().set_return_int_value(
        m_call_conv, *s,
        expr::ExprBuilder::The().mk_ite(
            res_expr, expr::ExprBuilder::The().mk_const(0, 8),
            expr::ExprBuilder::The().mk_const(1, 8)));
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
    auto str_addr =
        std::static_pointer_cast<const expr::ConstExpr>(str)->val().as_u64();
    auto resolved_strings = resolve_string(s, str_addr, max_forks);

    for (const auto& e : resolved_strings) {
        s->arch().set_return_int_value(
            m_call_conv, *e.state,
            expr::ExprBuilder::The().mk_const(e.str->size() / 8UL - 1UL,
                                              s->arch().ptr_size()));
        s->arch().handle_return(e.state, o_successors);
    }
}

void strncpy::exec(state::StatePtr           s,
                   executor::ExecutorResult& o_successors) const
{
    auto dst = s->get_int_param(m_call_conv, 0);
    auto src = s->get_int_param(m_call_conv, 1);
    auto n   = s->get_int_param(m_call_conv, 2);

    if (dst->kind() != expr::Expr::Kind::CONST) {
        err("strncpy") << "dst is symbolic" << std::endl;
        exit_fail();
    }
    if (src->kind() != expr::Expr::Kind::CONST) {
        err("strncpy") << "src is symbolic" << std::endl;
        exit_fail();
    }
    if (n->kind() != expr::Expr::Kind::CONST) {
        err("strncpy") << "n is symbolic (fixme)" << std::endl;
        exit_fail();
    }

    s->arch().set_return_int_value(m_call_conv, *s, dst);
    auto dst_ =
        std::static_pointer_cast<const expr::ConstExpr>(dst)->val().as_u64();
    auto src_ =
        std::static_pointer_cast<const expr::ConstExpr>(src)->val().as_u64();
    auto n_ =
        std::static_pointer_cast<const expr::ConstExpr>(n)->val().as_u64();

    int  max_forks        = 32;
    auto resolved_strings = resolve_string(s, src_, max_forks, n_);

    for (const auto& e : resolved_strings) {
        e.state->write_buf(dst_, e.str);
        s->arch().handle_return(e.state, o_successors);
    }
}

void strcpy::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto dst = s->get_int_param(m_call_conv, 0);
    auto src = s->get_int_param(m_call_conv, 1);

    if (dst->kind() != expr::Expr::Kind::CONST) {
        err("strcpy") << "dst is symbolic" << std::endl;
        exit_fail();
    }
    if (src->kind() != expr::Expr::Kind::CONST) {
        err("strcpy") << "src is symbolic" << std::endl;
        exit_fail();
    }

    s->arch().set_return_int_value(m_call_conv, *s, dst);
    auto dst_ =
        std::static_pointer_cast<const expr::ConstExpr>(dst)->val().as_u64();
    auto src_ =
        std::static_pointer_cast<const expr::ConstExpr>(src)->val().as_u64();

    int  max_forks        = 32;
    auto resolved_strings = resolve_string(s, src_, max_forks);

    for (const auto& e : resolved_strings) {
        e.state->write_buf(dst_, e.str);
        s->arch().handle_return(e.state, o_successors);
    }
}

void strcat::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto dst = s->get_int_param(m_call_conv, 0);
    auto src = s->get_int_param(m_call_conv, 1);

    // info("strcat") << dst->to_string() << " " << src->to_string() << std::endl;

    if (dst->kind() != expr::Expr::Kind::CONST) {
        err("strcat") << "dst is symbolic" << std::endl;
        exit_fail();
    }
    if (src->kind() != expr::Expr::Kind::CONST) {
        err("strcat") << "src is symbolic" << std::endl;
        exit_fail();
    }

    s->arch().set_return_int_value(m_call_conv, *s, dst);
    auto dst_ =
        std::static_pointer_cast<const expr::ConstExpr>(dst)->val().as_u64();
    auto src_ =
        std::static_pointer_cast<const expr::ConstExpr>(src)->val().as_u64();

    int  max_forks            = 8;
    auto resolved_src_strings = resolve_string(s, src_, max_forks);

    for (const auto& e : resolved_src_strings) {
        auto resolved_dst_strings = resolve_string(e.state, dst_, max_forks);
        for (const auto& ee : resolved_dst_strings) {
            // std::cout << "str1: " << e.str->to_string()
            //           << " | str2: " << ee.str->to_string() << std::endl;
            e.state->write_buf(dst_ + ee.str->size() / 8UL, e.str);
            s->arch().handle_return(ee.state, o_successors);
        }
    }
}

} // namespace naaz::models::libc
