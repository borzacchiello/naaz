#include "directory.hpp"

#include "../../expr/Expr.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"
#include "../../util/strutil.hpp"

namespace naaz::models::libc
{

void read::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    // FIXME: handle symbolic buffer and size

    auto fd   = s->get_int_param(m_call_conv, 0);
    auto buf  = s->get_int_param(m_call_conv, 1);
    auto size = s->get_int_param(m_call_conv, 2);

    if (fd->kind() != expr::Expr::Kind::CONST) {
        err("read") << "file descriptor is symbolic" << std::endl;
        exit_fail();
    }
    if (buf->kind() != expr::Expr::Kind::CONST) {
        err("read") << "buffer address is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("read") << "size is symbolic (FIXME)" << std::endl;
        exit_fail();
    }

    int fd_ =
        std::static_pointer_cast<const expr::ConstExpr>(fd)->val().as_s64();
    uint64_t buf_ =
        std::static_pointer_cast<const expr::ConstExpr>(buf)->val().as_u64();
    size_t size_ =
        std::static_pointer_cast<const expr::ConstExpr>(size)->val().as_u64();

    auto data = s->fs().read(fd_, size_);
    s->write_buf(buf_, data);
    s->arch().handle_return(s, o_successors);
}

void open::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    // FIXME: other parameters are ignored

    auto path = s->get_int_param(m_call_conv, 0);
    if (path->kind() != expr::Expr::Kind::CONST) {
        err("open") << "file path (pointer) is symbolic" << std::endl;
        exit_fail();
    }

    uint64_t path_ptr =
        std::static_pointer_cast<const expr::ConstExpr>(path)->val().as_u64();

    std::string path_name;
    uint64_t    off = 0;
    while (1) {
        auto b = s->read(path_ptr + off, 1);
        if (b->kind() != expr::Expr::Kind::CONST) {
            err("open") << "file path is symbolic" << std::endl;
            exit_fail();
        }
        auto b_ = std::static_pointer_cast<const expr::ConstExpr>(b);
        if (b_->val().as_s64() == 0)
            break;
        path_name.push_back((char)b_->val().as_s64());

        off += 1;
    }

    int fd = s->fs().open(path_name);
    s->arch().set_return_int_value(m_call_conv, *s,
                                   expr::ExprBuilder::The().mk_const(fd, 32));
    s->arch().handle_return(s, o_successors);
}

void close::exec(state::StatePtr           s,
                 executor::ExecutorResult& o_successors) const
{
    auto fd = s->get_int_param(m_call_conv, 0);

    if (fd->kind() != expr::Expr::Kind::CONST) {
        err("read") << "file descriptor is symbolic" << std::endl;
        exit_fail();
    }

    int fd_ =
        std::static_pointer_cast<const expr::ConstExpr>(fd)->val().as_s64();
    s->fs().close(fd_);

    s->arch().handle_return(s, o_successors);
}

} // namespace naaz::models::libc
