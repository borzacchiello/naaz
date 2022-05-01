#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::libc
{

static uint64_t resolve_size(state::StatePtr s, expr::ExprPtr size)
{
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("State")
            << "*alloc::resolve_size(): symbolic size is not supported (FIXME)"
            << std::endl;
        exit_fail();
    }
    return std::static_pointer_cast<const expr::ConstExpr>(size)
        ->val()
        .as_u64();
}

void malloc::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto size  = s->get_int_param(m_call_conv, 0);
    auto size_ = resolve_size(s, size);
    auto ptr   = s->allocate(size_);

    s->arch().set_return_int_value(
        m_call_conv, *s,
        expr::ExprBuilder::The().mk_const(ptr, s->arch().ptr_size()));
    s->arch().handle_return(s, o_successors);
}

void calloc::exec(state::StatePtr           s,
                  executor::ExecutorResult& o_successors) const
{
    auto nmemb = s->get_int_param(m_call_conv, 0);
    auto size  = s->get_int_param(m_call_conv, 1);
    auto size_ = resolve_size(s, size) * resolve_size(s, nmemb);

    auto ptr = s->allocate(size_);
    for (uint64_t i = 0; i < size_; ++i)
        s->write(ptr + i, expr::ExprBuilder::The().mk_const(0, 8));

    s->arch().set_return_int_value(
        m_call_conv, *s,
        expr::ExprBuilder::The().mk_const(ptr, s->arch().ptr_size()));
    s->arch().handle_return(s, o_successors);
}

void realloc::exec(state::StatePtr           s,
                   executor::ExecutorResult& o_successors) const
{
    // FIXME: handle corner cases (e.g., old_ptr == 0, or size == 0, etc.)

    auto old_ptr = s->get_int_param(m_call_conv, 0);
    auto size    = s->get_int_param(m_call_conv, 1);
    auto size_   = resolve_size(s, size);

    auto ptr  = s->allocate(size_);
    auto data = s->read_buf(old_ptr, size_);
    s->write_buf(ptr, data);

    s->arch().set_return_int_value(
        m_call_conv, *s,
        expr::ExprBuilder::The().mk_const(ptr, s->arch().ptr_size()));
    s->arch().handle_return(s, o_successors);
}

void free::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
