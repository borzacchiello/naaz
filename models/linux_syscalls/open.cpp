#include "directory.hpp"
#include "../../expr/ExprBuilder.hpp"
#include "../../state/State.hpp"
#include "../../util/ioutil.hpp"

namespace naaz::models::linux_syscalls
{

void open::exec(state::StatePtr s, executor::ExecutorResult& o_successors) const
{
    // FIXME: other parameters are ignored
    auto path = s->get_syscall_param(0);
    if (path->kind() != expr::Expr::Kind::CONST) {
        err("sys_open") << "file path (pointer) is symbolic" << std::endl;
        exit_fail();
    }

    uint64_t path_ptr =
        std::static_pointer_cast<const expr::ConstExpr>(path)->val().as_u64();

    std::string path_name;
    uint64_t    off = 0;
    while (1) {
        auto b = s->read(path_ptr + off, 1);
        if (b->kind() != expr::Expr::Kind::CONST) {
            err("sys_open") << "file path is symbolic" << std::endl;
            exit_fail();
        }
        auto b_ = std::static_pointer_cast<const expr::ConstExpr>(b);
        if (b_->val().as_s64() == 0)
            break;
        path_name.push_back((char)b_->val().as_s64());

        off += 1;
    }

    int fd = s->fs().open(path_name);
    s->arch().set_syscall_return_value(
        *s, expr::ExprBuilder::The().mk_const(fd, 32));
}

} // namespace naaz::models::linux_syscalls
