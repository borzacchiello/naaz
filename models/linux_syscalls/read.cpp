#include "directory.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::linux_syscalls
{

void read::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    auto fd   = s->get_syscall_param(0);
    auto buf  = s->get_syscall_param(1);
    auto size = s->get_syscall_param(2);

    if (fd->kind() != expr::Expr::Kind::CONST) {
        err("sys_read") << "file descriptor is symbolic" << std::endl;
        exit_fail();
    }
    if (buf->kind() != expr::Expr::Kind::CONST) {
        err("sys_read") << "buffer address is symbolic (FIXME)" << std::endl;
        exit_fail();
    }
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("sys_read") << "size is symbolic (FIXME)" << std::endl;
        exit_fail();
    }

    int fd_ =
        std::static_pointer_cast<const expr::ConstExpr>(fd)->val().as_s64();
    uint64_t buf_ =
        std::static_pointer_cast<const expr::ConstExpr>(buf)->val().as_u64();
    size_t size_ =
        std::static_pointer_cast<const expr::ConstExpr>(size)->val().as_u64();

    auto data = s->fs().read(fd_, size_);
    if (fd_ == 0 && g_config.printable_stdin) {
        for (size_t i = 0; i < size_; ++i) {
            auto b =
                expr::ExprBuilder::The().mk_extract(data, i * 8 + 7, i * 8);
            auto l = expr::ExprBuilder::The().mk_const(0x20ul, 8);
            auto h = expr::ExprBuilder::The().mk_const(0x7eul, 8);
            s->solver().add(expr::ExprBuilder::The().mk_uge(b, l));
            s->solver().add(expr::ExprBuilder::The().mk_ule(b, h));
        }
    }

    s->write_buf(buf_, data);
    s->arch().set_syscall_return_value(
        *s, expr::ExprBuilder::The().mk_const(size_, s->arch().ptr_size()));
}

} // namespace naaz::models::linux_syscalls
